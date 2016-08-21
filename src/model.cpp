#include "mswin.h"
#include "model.h"
#include "compiler.h"
#include "memory.h"
#include "kdtree.h"
#include "random.h"
#include "rodrigues.h"
#include "markov.h"
#include "random-util.h"
#include "partition.h"
#include "graphics.h"
#include "aligned-arrays.h"
#include "vector.h"
#include "hsv-to-rgb.h"
#include "print.h"

#include <algorithm>

namespace usr {
  // Physical parameters.
  static const float density = 100.0f; // Density of a ball.

  // Pixels per logical distance unit (at front of tank).
  static const float scale = 50.0f;

  // Logical time units per frame.
  static const float frame_time = 1.0f / 60.0f;

  static const float alpha = 0.95f;    // Alpha of output fragments.
  static const float fog_near = 0.0f;  // Fog blend factor at near plane.
  static const float fog_far = 0.8f;   // Fog blend factor at far plane.

  static const float line_width_extra = 0.0f;
  static const float line_sharpness = 1.0f;

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

  //                                             t0     t1     t2     t3     v0     v1
  static const bump_specifier_t hsv_s_bump = { 1.50f, 1.75f, 3.75f, 4.25f, 0.00f, 0.25f, };
  static const bump_specifier_t hsv_v_bump = { 1.50f, 1.75f, 3.75f, 4.25f, 0.07f, 0.25f, };

  // Parameters for morph animation timings.
  static const float morph_start = 1.75f;
  static const float morph_finish = 3.50f;
  static const float cycle_duration = 4.25f;
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

model_t::model_t () : memory (nullptr), capacity (0), count (0) { }

inline float rainbow_hue (float x)
{
  // This is the degree-5 minimax polynomial on [0, 1] for
  // the piecewise linear function on [0,1] joining points
  // (0, 0.955), (1/2, 1.230), (2/3, 1.420) and (1, 1.955).
  static const v4f poly_lo = { +0x1.e31f38P-1f, +0x1.3caffeP+0f, -0x1.37390cP+2f, +0x1.9c9bbaP+3f, };
  static const v4f poly_hi = { -0x1.857638P+3f, +0x1.f3840cP+1f, 0.0f, 0.0f, };
  x -= (int) x; // fractional part, assuming non-negative
  return polyeval7 (x, poly_lo, poly_hi);
}

bool model_t::start (int width, int height, const settings_t & settings)
{
  radius = 0.5f + 0.01f * ui2f (settings.trackbar_pos [3]);
  ALIGNED16 float view [4];

  float scale = 0.5f / usr::scale;
  // x1, y1, z1: coordinates of bottom-right-front corner of view frustum.
  float x1 = view [0] = scale * width;
  float y1 = view [1] = scale * height;
  float z1 = view [2] = scale * (-3 * std::max (width, height));

  float zd = std::min (x1, y1); // td: depth of frustum
  // x2, y2, z2: coordinates of bottom-right-back corner of view frustum.
  float z2 = view [3] = z1 - zd;
  float x2 = x1 * (z2 / z1);
  float y2 = y1 * (z2 / z1);

  program.set_view (view, width, height,
                    usr::fog_near, usr::fog_far,
                    usr::line_width_extra, usr::line_sharpness);

  // Calculate wall planes to exactly fill the front of the viewing frustum.

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

  // Add objects.

  // Trackbar positions 0, 1, 2 specify 1, 2, 3 objects respectively.
  // subsequently the number of objects increases linearly with position.
  unsigned pos = settings.trackbar_pos [0];
  unsigned max_count = std::max (3u, truncate ((x1 / (2 * radius) + 1) *
                                               (y1 / (2 * radius) + 1) *
                                               (zd / (2 * radius) + 1)));
  count = pos < 2 ? pos + 1 : 3 + (max_count - 3) * (pos - 2) / 98;
  set_capacity (count);

  float mass = usr::density * (radius * radius);
  float moment = 0.4f * usr::density * ((radius * radius) * (radius * radius));

  for (unsigned n = 0; n != count; ++ n) {
    kdtree_index [n] = n;
    object_order [n] = n;
    object_t & A = objects [n];
    A.r = radius;
    A.m = mass;
    A.l = moment;
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

    std::uint64_t entropy = rng.get ();
    // Don't use 64-bit modulo in 32-bit environments, as it's provided by libc.
    A.target.system = (system_select_t) ((std::size_t) (entropy >> 3) % 6);
    A.target.point = entropy & 7;
    A.starting_point = A.target.point;
  }

  float global_phase_offset = get_float (rng, 1.0f, 2.0f);
  for (unsigned n = 0; n != count; ++ n) {
    float phase = ui2f (n) / ui2f (count);
    objects [n].hue = rainbow_hue (global_phase_offset - phase);
    // Initial animation_time is in [T, 2T) to force an immediate transition.
    objects [n].animation_time = (1.0f + phase) * usr::cycle_duration;
  }

  // Take the animation-speed setting, s, a whole number in the range 0 to 100, inclusive;
  // every frame, the morph/fade animation time is advanced by the time interval kT, where
  // k is an increasing continuous function of s, and the constant T is the default frame time.
  DWORD s = settings.trackbar_pos [2];
  // Boost sensitivity in upper range. (Note: 100 to power 4 < 2 to power 27.)
  float k = s <= 50 ? 0.02f * ui2f (s) : 0.00000016f * ui2f ((s * s) * (s * s));
  animation_speed_constant = k;

  // Initialize bump function object for the lightness and saturation fading animation.
  ALIGNED16 bump_specifier_t s_bump = usr::hsv_s_bump;
  ALIGNED16 bump_specifier_t v_bump = usr::hsv_v_bump;
  if (s > 50) {
    // At higher animation speed, progressively suppress fading.
    DWORD s1 = 100 - s;
    float g = 0.02f * ui2f (s1);              // Saturation fading decreases linearly.
    float h = g * g * g;                      // Lightness fading falls off more rapidly.
    if (s >= 75) { g = 0.0f; h = 0.0f; }      // No fading at all above 75% speed.
    s_bump.v0 = g * s_bump.v0 + (1.0f - g) * s_bump.v1;
    v_bump.v0 = h * v_bump.v0 + (1.0f - h) * v_bump.v1;
  }
  if (s <= 30) {
    // At low speed, offset the attack-begin and attack-end times
    // of the lightness and saturation independently to give a white
    // warning flash effect.
    s_bump.t0 += 0.10f;
    s_bump.t1 += 0.25f;
    v_bump.t1 -= 0.15f;
  }
  bumps.initialize (s_bump, v_bump);

  // Allow the balls to jostle for space.
  for (unsigned n = 0; n != 24; ++ n) nodraw_next ();

  // Slow down to the configured speed.
  v4f slowdown = _mm_set1_ps (0.005f * ui2f (settings.trackbar_pos [1]));
  for (unsigned n = 0; n != count; ++ n) {
    store4f (v [n], slowdown * load4f (v [n]));
    store4f (w [n], slowdown * load4f (w [n]));
  }
  return true;
}

bool model_t::initialize (std::uint64_t seed)
{
  if (! initialize_graphics (program)) return false;
  rng.initialize (seed);
  step.initialize (usr::morph_start, usr::morph_finish);
  initialize_systems (abc, xyz, xyzinvt, primitive_count, vao_ids);
  return true;
}

model_t::~model_t ()
{
#if PRINT_ENABLED
  std::cout << std::scientific << std::setprecision (8);
  std::cout << "Required range for arccos function: x in [" << min_d << ", " << max_d << "].\n\n";

  double total_polyhedron_count = 0.0;
  for (unsigned n = 0; n != 18; ++ n) {
    total_polyhedron_count += polyhedron_counts [n];
  }

  std::cout << std::fixed << std::setprecision (2);
  for (unsigned n = 0; n != 18; ++ n) {
    std::cout << std::setw (10) << (100.0f * polyhedron_counts [n] / total_polyhedron_count) << " % " << names [n] << "\n";
  }
#endif
  //deallocate (memory);
}

void model_t::set_capacity (std::size_t new_capacity)
{
  reallocate_aligned_arrays (memory, capacity, new_capacity,
                             & x, & v, & u, & w, & e,
                             & kdtree_index,
                             & objects,
                             & object_order);
}

void model_t::recalculate_locus (unsigned index)
{
  object_t & object = objects [index];
  float (& locus_end) [4] = e [index];

  system_select_t sselect = object.target.system;
  v4f g0 = load4f (abc [sselect] [object.starting_point]); // g0: Coefficients for T0 in terms of xyz.
  v4f g1 = load4f (abc [sselect] [object.target.point]);   // g1: Coefficients for T1 in terms of xyz.
  v4f T0 = tmapply (xyz [sselect], g0);                    // T0: Beginning of locus (unit vector).
  v4f T1 = tmapply (xyz [sselect], g1);                    // T1: End of locus (also a unit vector).
  // Find a unit vector T2 perpendicular to T0 and and an angle s such that T1 = (cos s) T0 + (sin s) T2.
  v4f d = dot (T0, T1);                                    // d:  Dot product of T0 and T1 (i.e., cos s, where s is the arc-length T0-T1).
  v4f T2 = normalize (T1 - d * T0);                        // T2: Take the component of T1 perpendicular to T0, then normalize.
  v4f g2 = mapply (xyzinvt [sselect], T2);                 // g2: Coefficents for T2 in terms of xyz.
  // Any point T = (cos t) T0 + (cos t) T2 (where 0 <= t <= s) on the arc T0-T1 has coefficients (cos t) g0 + (sin t) g2.
  store4f (locus_end, g2);
  // Calculate the arc-length s = arccos(d) of the great circular arc T0-T1.
  // Note: for allowed transitions we have d in [7.74596691e-001, 9.90879238e-001].
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
    kdtree.compute (kdtree_index, x, count); // undefined behaviour if count == 0.
    kdtree.search (kdtree_index, x, count, walls, radius, objects, v, w);
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
      ++ polyhedron_counts [polyhedra [(int) A.target.system] [A.target.point] - 1];
#endif
    }
    A.animation_time = t;
  }

  // Sort objects into reverse depth order. Insertion sort is slow the
  // first time, but faster for subsequent frames because the index is
  // already almost sorted.
  if (count) {
    insertion_sort (object_order, x, 2, 0, count); // undefined behaviour if count == 0.
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
  compute (reinterpret_cast <char *> (& uniform_buffer [0].m), uniform_buffer.stride (), x, u, & (object_order [begin]), count);

  const v4f alpha = { 0.0f, 0.0f, 0.0f, usr::alpha, };
  for (unsigned n = 0; n != count; ++ n) {
    unsigned m = object_order [begin + n];
    const object_t & object = objects [m];
    object_data_t & block = uniform_buffer [n];

    // Snub?
    block.s = (GLuint) (object.starting_point == 7 || object.target.point == 7);

    // Set the diffuse material reflectance, d.
    v4f satval = bumps (object.animation_time);
    v4f sat = _mm_moveldup_ps (satval);
    v4f val = _mm_movehdup_ps (satval);
    _mm_stream_ps (block.d, hsv_to_rgb (object.hue, sat, val, alpha));

    // Set the vertex coefficients g.
    system_select_t sselect = object.target.system;
    v4f t = step (object.animation_time) * _mm_set1_ps (object.locus_length);
    v4f sc = sincos (t);
    v4f s = _mm_moveldup_ps (sc);
    v4f c = _mm_movehdup_ps (sc);
    v4f g = c * load4f (abc [sselect] [object.starting_point]) + s * load4f (e [m]);
    _mm_stream_ps (block.g, _mm_set1_ps (object.r) * g);
  }

  uniform_buffer.update ();

  for (unsigned n = 0; n != count; ++ n) {
    unsigned m = object_order [begin + n];
    const object_t & object = objects [m];
    system_select_t sselect = object.target.system;

    paint (primitive_count [sselect],
           vao_ids [sselect],
           uniform_buffer.id (),
           (std::uint32_t) (n * uniform_buffer.stride ()));
  }
}
