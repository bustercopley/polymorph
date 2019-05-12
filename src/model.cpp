// Copyright 2012-2019 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mswin.h"

#include "model.h"
#include "aligned-arrays.h"
#include "bounce.h"
#include "hsv-to-rgb.h"
#include "kdtree.h"
#include "markov.h"
#include "memory.h"
#include "partition.h"
#include "print.h"
#include "random-util.h"
#include "rodrigues.h"
#include "vector.h"
#include <algorithm>

__attribute__ ((optimize ("O3"))) float cube (float x)
{
  // As of GCC 7.1, with -Os and -ffast-math, for "x * x * x" the
  // compiler emits a call to the powi function instead of the two
  // multiplications, which is inconvenient under -nostdlib.
  return x * x * x;
}

namespace usr {
  // Physical parameters.
  const float density = 100.0f;      // Density of a ball.
  const float fill_factor = 0.185f;  // Density of the gas.

  // Pixels per logical distance unit (at front of tank).
  const float scale = 50.0f;

  // Logical time units per frame.
  const float frame_time = 1.0f / 60.0f;

  const float alpha = 0.85f;    // Alpha of output fragments.
  const float fog_near = 0.0f;  // Fog blend factor at near plane.
  const float fog_far = 0.8f;   // Fog blend factor at far plane.

  const float line_width_extra = 0.0f;
  const float line_sharpness = 1.0f;

  // Parameters for material-colour animation timings.

  //  r ^                        r = bump (t)
  //    |
  // v1 +--------------XXXXXX----------------
  //    |           X  |    |  X
  //    |         X    |    |    X
  //    |        X     |    |     X
  //    |      X       |    |       X
  // v0 +XXXX----------+----+----------XXXX-->
  //        t0        t1    t2        t3      t

  //                                 t0     t1     t2     t3     v0     v1
  const bump_specifier_t sbump = { 1.50f, 1.75f, 3.75f, 4.25f, 0.00f, 0.275f };
  const bump_specifier_t vbump = { 1.50f, 1.75f, 3.75f, 4.25f, 0.09f, 0.333f };

  // Parameters for morph animation timings.
  const float morph_start = 1.75f;
  const float morph_finish = 3.50f;
  const float cycle_duration = 4.25f;
}

#if PRINT_ENABLED
// These are Father Wenninger's numbers.
const unsigned polyhedra [system_count] [8] = {
  { 2, 1, 1, 11, 6, 6, 7, 4, },
  { 2, 1, 1, 11, 6, 6, 7, 4, },
  { 11, 2, 3, 13, 8, 7, 15, 17, },
  { 11, 2, 3, 13, 8, 7, 15, 17, },
  { 12, 4, 5, 14, 10, 9, 16, 18, },
  { 12, 4, 5, 14, 10, 9, 16, 18, },
};

unsigned polyhedron_counts [18] = { 0 };

const char * names [] = {
  "tetrahedron",
  "octahedron",
  "cube",
  "icosahedron",
  "dodecahedron",
  "truncated tetrahedron",
  "truncated octahedron",
  "truncated cube",
  "truncated icosahedron",
  "truncated dodecahedron",
  "cuboctahedron",
  "icosidodecahedron",
  "rhombicuboctahedron",
  "rhombicosidodecahedron",
  "rhombitruncated cuboctahedron",
  "rhombitruncated icosidodecahedron",
  "snub cube",
  "snub dodecahedron",
};

float min_d = 1.0f, max_d = 0.0f;
#endif

// Argument: t in [0, 1]; result: a hue in [0, 6].
inline float rainbow_hue (float x)
{
  // Define a path along the colour-hexagon defined in "hsv-to-rgb.h"
  // which progresses regularly through the classical seven colours of
  // the rainbow (roughly).

  // Let p be the piecewise linear function on [0,1] joining points
  // (0, 0.955), (1/2, 1.230), (2/3, 1.420) and (1, 1.955). There is
  // nothing very special about these four points; they were chosen
  // experimentally.

  // Return f(x), where f is the degree-7 minimax polynomial for p on
  // [0, 1]. The error f(x)-p(x) is equioscillating and of degree 8,
  // and attains extrema at the endpoints. Therefore, the errors at
  // the endpoints are equal in sign and magnitude; hence
  // f(1)-f(0) = (p(1)+E) - (p(0)+E) = p(1)-p(0) = 1.955-0.955 = 1.

  // Seen as a graph of (angular position)/2pi vs. time, f(x) describes
  // a one-revolution rotation at variable speed over one unit of time.

  const v4f poly_lo = {
    +0x1.f2b75cP-1f, +0x1.d75e6cP-9f, +0x1.55b804P+3f, -0x1.0a7118P+6f
  };
  const v4f poly_hi = {
    +0x1.811250P+7f, -0x1.1565f6P+8f, +0x1.86704aP+7f, -0x1.ab6de6P+5f
  };
  x -= (int) x; // Fractional part, assuming non-negative.
  return polyeval7 (x, poly_lo, poly_hi);
}

bool model_t::start (int width, int height, const settings_t & settings)
{
  ALIGNED16 float view [4];

  float scale = 0.5f / usr::scale;
  float sharpness = usr::line_sharpness;
  float fwidth = width;
  float fheight = height;
  // Adjust scale and line width for small windows (for parented mode).
  if (width < 512) scale *= 512.0f / fwidth;
  if (width < 256) sharpness *= 256.0f / fwidth;

  // x1, y1, z1: Coordinates of bottom-right-front corner of view frustum.
  float x1 = view [0] = scale * fwidth;
  float y1 = view [1] = scale * fheight;
  float z1 = view [2] = -3.0f * std::max (x1, y1);

  float zd = std::min (x1, y1); // zd: Depth of view frustum.
  // x2, y2, z2: Coordinates of bottom-right-back corner of view frustum.
  float z2 = view [3] = z1 - zd;
  float x2 = x1 * (z2 / z1);
  float y2 = y1 * (z2 / z1);

  program.set_view (view, width, height,
                    usr::fog_near, usr::fog_far,
                    usr::line_width_extra, sharpness);

  // Calculate wall planes to fit the front of the viewing frustum.
  ALIGNED16 const float temp [6] [2] [4] = {
    { { 0.0f, 0.0f, z1, 0.0f, }, { 0.0f, 0.0f, -1.0f, 0.0f, }, },
    { { 0.0f, 0.0f, z2, 0.0f, }, { 0.0f, 0.0f,  1.0f, 0.0f, }, },
    { {  -x1, 0.0f, z1, 0.0f, }, { z1 - z2, 0.0f, x1 - x2, 0.0f, }, },
    { {   x1, 0.0f, z1, 0.0f, }, { z2 - z1, 0.0f, x1 - x2, 0.0f, }, },
    { { 0.0f,  -y1, z1, 0.0f, }, { 0.0f, z1 - z2, y1 - y2, 0.0f, }, },
    { { 0.0f,   y1, z1, 0.0f, }, { 0.0f, z2 - z1, y1 - y2, 0.0f, }, },
  };

  for (unsigned k = 0; k != 6; ++ k) {
    v4f anchor = load4f (temp [k] [0]);
    v4f normal = load4f (temp [k] [1]);
    store4f (walls [k] [0], anchor);
    store4f (walls [k] [1], normalize (normal));
  }

  // Object circumradius is in [0.5, 1.5).
  radius = 0.5f + 0.01f * ui2f (settings.trackbar_pos [3]);
  float rsq = radius * radius;
  float mass = usr::density * rsq;
  float moment = 0.4f * usr::density * (rsq * rsq);
  float phase_offset = get_float (rng, 1.0f, 2.0f);
  // Trackbar positions 0, 1, 2 specify 1, 2, 3 objects respectively;
  // subsequently the number of objects increases linearly with position.
  unsigned max_count = std::max (3u,
    truncate (usr::fill_factor * (x1 * y1 * zd) / (rsq * radius)));
  DWORD pos = settings.trackbar_pos[0];
  count = pos < 2 ? pos + 1 : 3 + (max_count - 3) * (pos - 2) / 98;
  set_capacity (count);

  // Add objects.
  for (unsigned n = 0; n != count; ++ n) {
    kdtree_index [n] = n;
    object_order [n] = n;

    v4f c = { 0.0f, 0.0f, 0.5f * (z1 + z2), 0.0f, };
    v4f m = { x2 - radius, y2 - radius, 0.5f * (z1 - z2) - radius, 0.0f, };
  loop:
    // Get a random point in the bounding cuboid of the viewing frustum.
    v4f t = m * get_vector_in_box (rng) + c;
    // Discard and try again if distance to any wall is less than radius.
    // No need to check the first two walls (the front and rear).
    for (unsigned k = 2; k != 6; ++ k) {
      v4f anchor = load4f (walls [k] [0]);
      v4f normal = load4f (walls [k] [1]);
      float s = _mm_cvtss_f32 (dot (t - anchor, normal));
      if (s < radius) {
        goto loop;
      }
    }
    // Moderately high initial temperature for rapid annealing.
    store4f (x [n], t);
    store4f (v [n], get_vector_in_ball (rng, 0.25f));
    store4f (u [n], get_vector_in_ball (rng, 0x1.921fb4P+001f)); // pi
    store4f (w [n], get_vector_in_ball (rng, 0.10f));

    object_t & A = objects [n];
    A.m = mass;
    A.l = moment;
    A.r = radius;

    float phase = ui2f (n) / ui2f (count);
    A.hue = rainbow_hue (phase_offset - phase);
    // Initial animation_time is in [T, 2T) to force an immediate transition.
    A.animation_time = (1.0f + phase) * usr::cycle_duration;

    // Get independent integers, d6 uniform on [0, 6) and d8 uniform on [0, 8).
    std::uint32_t d = (std::uint32_t) rng.get ();
    std::uint32_t d8 = d & 7u;
    std::uint32_t d6 = (3u * (d >> 3u)) >> 28u;
    A.target.system = (system_select_t) d6;
    A.target.point = d8;
    A.starting_point = A.target.point;
  }

  // Take the animation-speed s, an integer in the range 0 to 100, inclusive;
  // every frame, the morph/fade animation time is advanced by the time interval
  // kT, where k is an increasing continuous function of s, and the constant T
  // is the default frame time.
  DWORD s = settings.trackbar_pos [2];
  float k = 0.02f * ui2f (s);  // 0.0 <= k <= 2.0
  // Boost animation speed in the upper half of the range.
  animation_speed_constant = s <= 50 ? k : ((k * k) * (k * k));

  // Initialize bump functions for the lightness and saturation fade animation.
  ALIGNED16 bump_specifier_t sbump = usr::sbump;
  ALIGNED16 bump_specifier_t vbump = usr::vbump;
  if (s >= 75) {
    // No fading at all above 75% animation speed.
    sbump.v0 = sbump.v1;
    vbump.v0 = vbump.v1;
  }
  else if (s >= 50) {
    // Between 50% and 75% animation speed, progressively suppress fading.
    float g = 2.0f - k;      // Saturation fading decreases linearly.
    float h = cube (g);      // Lightness fading falls off more rapidly.
    sbump.v0 = g * sbump.v0 + (1.0f - g) * sbump.v1;
    vbump.v0 = h * vbump.v0 + (1.0f - h) * vbump.v1;
  }
  if (s <= 30) {
    // At low animation speed, offset the attack-begin and attack-end
    // times of the lightness and saturation independently to give a
    // white warning flash effect.
    sbump.t0 += 0.10f;
    sbump.t1 += 0.25f;
    vbump.t1 -= 0.15f;
  }
  bumps.initialize (sbump, vbump);

  // Allow the balls to jostle for space.
  for (unsigned n = 0; n != 24; ++ n) nodraw_next ();

  // Slow down to the configured speed.
  s = settings.trackbar_pos [1];
  v4f speedup = _mm_set1_ps (
    ui2f (s) * (s <= 50 ? (0.125f / 50) : (0.125f / (50 * 50)) * ui2f (s)));
  for (unsigned n = 0; n != count; ++n) {
    store4f (v [n], speedup * load4f (v [n]));
    store4f (w [n], speedup * load4f (w [n]));
  }

  // Sort in reverse depth order for painter's algorithm (maintained with
  // insertion_sort in draw_next).
  qsort (object_order, x, 2, 0, count);

  return true;
}

bool model_t::initialize (std::uint64_t seed)
{
  if (! initialize_graphics (program)) return false;
  rng.initialize (seed);
  step.initialize (usr::morph_start, usr::morph_finish);
  initialize_systems (abc, xyz, xyzinv, primitive_count, vao_ids);
  return true;
}

model_t::~model_t ()
{
#if PRINT_ENABLED
  std::cout << std::scientific << std::setprecision (8)
            << "Required range for arccos function: x in [" << min_d << ", "
            << max_d << "].\n\n";

  double total_polyhedron_count = 0.0;
  for (unsigned n = 0; n != 18; ++ n) {
    total_polyhedron_count += polyhedron_counts [n];
  }

  std::cout << std::fixed << std::setprecision (2);
  for (unsigned n = 0; n != 18; ++ n) {
    std::cout << std::setw (10)
              << (100.0f * polyhedron_counts[n] / total_polyhedron_count)
              << " % " << names[n] << "\n";
  }
#endif
  //deallocate (memory);
  //deallocate (kdtree_split);
}

void model_t::set_capacity (std::size_t new_capacity)
{
  reallocate_aligned_arrays (memory, capacity, new_capacity, x, v, u, w, e,
    kdtree_index, kdtree_aux, objects, object_order);
}

void model_t::recalculate_locus (unsigned index)
{
  object_t & object = objects [index];
  float (& locus_end) [4] = e [index];

  system_select_t sselect = object.target.system;

  // g0: Coefficients for T0 in terms of xyz.
  // g1: Coefficients for T1 in terms of xyz.
  // T0: Beginning of locus (unit vector).
  // T1: End of locus (also a unit vector).
  // d:  Dot product of T0 and T1, i.e., cos s where s is the arc length T0-T1.
  // T2: Component of T1 perpendicular to T0, normalized to a unit vector.
  // g2: Coefficents for T2 in terms of xyz.

  v4f g0 = load4f (abc [sselect] [object.starting_point]);
  v4f g1 = load4f (abc [sselect] [object.target.point]);
  v4f T0 = mapply (xyz [sselect], g0);
  v4f T1 = mapply (xyz [sselect], g1);
  // Find a unit vector T2 perpendicular to T0 and and an angle s such that
  // T1 = (cos s) T0 + (sin s) T2.
  v4f d = dot (T0, T1);
  v4f T2 = normalize (T1 - d * T0);
  v4f g2 = mapply (xyzinv [sselect], T2);
  // Any point T = (cos t) T0 + (sin t) T2 (where 0 <= t <= s) on the
  // hemisemicircular arc T0-T1 has coefficients (cos t) g0 + (sin t) g2.
  store4f (locus_end, g2);
  // Calculate the arc-length s = arccos(d) of the great circular arc T0-T1.
  // Note: for allowed transitions we have d in [0.774596691, 0.990879238].
  object.locus_length = _mm_cvtss_f32 (arccos (d));

#if PRINT_ENABLED
  float df = _mm_cvtss_f32 (d);
  if (min_d > df) min_d = df;
  if (max_d < df) max_d = df;
#endif
}

void model_t::nodraw_next ()
{
  // Advance the simulation without updating the angular position.
  if (count) {
    // Collision detection.
    kdtree_search ();
  }

  advance_linear (x, v, count);
}

void model_t::draw_next ()
{
  // Advance the simulation including the angular position.
  nodraw_next ();
  advance_angular (u, w, count);

  // Advance the animation by one frame.
  const float dt = animation_speed_constant * usr::frame_time;

  for (unsigned n = 0; n != count; ++ n) {
    object_t & A = objects [n];
    float t = A.animation_time + dt;
    if (t >= usr::cycle_duration) {
      t -= usr::cycle_duration;
      // We must perform a Markov transition.
      transition (rng, u [n], A.target, A.starting_point);
      recalculate_locus (n);
#if PRINT_ENABLED
      int wenninger_num = polyhedra [(int) A.target.system] [A.target.point];
      ++ polyhedron_counts [wenninger_num - 1];
#endif
    }
    A.animation_time = t;
  }

  if (count) {
    // Restore the z-order which we have just perturbed.
    // Insertion sort is an adaptive sort algorithm.
    insertion_sort (object_order, x, 2, 0, count);
  }

  clear ();

  // Draw all the shapes, one uniform buffer at a time, in reverse depth order.
  unsigned buffer_count = (unsigned) program.uniform_buffer.count ();
  unsigned begin = 0, end = buffer_count;
  while (end < count) {
    draw (begin, end - begin);
    begin = end;
    end = begin + buffer_count;
  }
  draw (begin, count - begin);
}

void model_t::draw (unsigned begin, unsigned count)
{
  uniform_buffer_t & uniform_buffer = program.uniform_buffer;

  // Set the modelview matrix, m.
  compute (reinterpret_cast <char *> (& uniform_buffer [0].m),
    uniform_buffer.stride (), x, u, & (object_order [begin]), count);

  const v4f alpha = { 0.0f, 0.0f, 0.0f, usr::alpha, };
  for (unsigned n = 0; n != count; ++ n) {
    unsigned m = object_order [begin + n];
    const object_t & obj = objects [m];
    object_data_t & block = uniform_buffer [n];

    // Snub?
    block.s = (GLuint) (obj.starting_point == 7 || obj.target.point == 7);

    // Set the diffuse material reflectance, d.
    v4f satval = bumps (obj.animation_time);
    v4f sat = _mm_moveldup_ps (satval);
    v4f val = _mm_movehdup_ps (satval);
    _mm_stream_ps (block.d, hsv_to_rgb (obj.hue, sat, val, alpha));

    // Set the vertex coefficients g.
    system_select_t sselect = obj.target.system;
    v4f t = step (obj.animation_time) * _mm_set1_ps (obj.locus_length);
    v4f sc = sincos (t);
    v4f s = _mm_moveldup_ps (sc);
    v4f c = _mm_movehdup_ps (sc);
    v4f g0 = load4f (abc [sselect] [obj.starting_point]);
    v4f g = c * g0 + s * load4f (e [m]);
    _mm_stream_ps (block.g, _mm_set1_ps (obj.r) * g);
  }

  uniform_buffer.update ();

  for (unsigned n = 0; n != count; ++ n) {
    unsigned m = object_order [begin + n];
    const object_t & obj = objects [m];
    system_select_t sselect = obj.target.system;

    paint (primitive_count [sselect],
           vao_ids [sselect],
           uniform_buffer.id (),
           (std::uint32_t) (n * uniform_buffer.stride ()));
  }
}
