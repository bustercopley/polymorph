#include "compiler.h"
#include "memory.h"
#include "kdtree.h"
#include "model.h"
#include "random.h"
#include "rodrigues.h"
#include "markov.h"
#include "random.h"
#include "partition.h"
#include "graphics.h"
#include "config.h"
#include "aligned-arrays.h"
#include "vector.h"
#include "hsv-to-rgb.h"
#include "print.h"

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

bool model_t::initialize (unsigned long long seed, int width, int height)
{
#if 0
  float tz = usr::tank_distance, td = usr::tank_depth, th = usr::tank_height;
  v4f view = { tz, td, (th * width) / height, th, };
#else
  typedef std::int32_t v4i __attribute__ ((vector_size (16), aligned(16)));
  v4i wi = { height, height, width, height, };
  v4f hw = _mm_cvtepi32_ps ((__m128i) wi);
  v4f hh = _mm_movelh_ps (hw, hw);
  v4f ratio = hw / hh;
  v4f vw = { usr::tank_distance, usr::tank_depth, usr::tank_height, usr::tank_height, };
  v4f view = _mm_mul_ps (vw, ratio);
#endif

  unsigned total_count = usr::fill_ratio * double (width) / height;
  if (! set_capacity (total_count)) return false;
  if (! uniform_buffer.initialize ()) return false;
  if (! initialize_programs (programs, view)) return false;
  rng.initialize (seed);

  // Calculate wall planes to exactly fill the front of the viewing frustum.
  float t [4] ALIGNED16;
  store4f (t, view);
  float z1 = -t [0];
  float z2 = -t [0] - t [1];
  float x1 = t [2] / 2;
  float y1 = t [3] / 2;
  float x2 = x1 * z2 / z1;
  float y2 = y1 * z2 / z1;
  // Now push the front wall back a little (avoids visual artifacts on some machines).
  z1 -= 0.1f;

  const float temp [6] [2] [4] ALIGNED16 = {
    { { 0.0f, 0.0f, z1, 0.0f, }, { 0.0f, 0.0f, -1.0f, 0.0f, }, },
    { { 0.0f, 0.0f, z2, 0.0f, }, { 0.0f, 0.0f,  1.0f, 0.0f, }, },
    { {  -x1, 0.0f, z1, 0.0f, }, { z1 - z2, 0.0f, x1 - x2, 0.0f, }, },
    { {   x1, 0.0f, z1, 0.0f, }, { z2 - z1, 0.0f, x1 - x2, 0.0f, }, },
    { { 0.0f,  -y1, z1, 0.0f, }, { 0.0f, z1 - z2, y1 - y2, 0.0f, }, },
    { { 0.0f,   y1, z1, 0.0f, }, { 0.0f, z2 - z1, y1 - y2, 0.0f, }, },
  };

  for (unsigned k = 0; k != 6; ++ k) {
    v4f anchor = load4f (temp [k] [0]);
    v4f normal = normalize (load4f (temp [k] [1]));
    store4f (walls [k] [0], anchor);
    store4f (walls [k] [1], normal);
  }

  max_radius = 0.0f;

  for (unsigned n = 0; n != total_count; ++ n) add_object (view);
  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;

  float phase = 0.0f;
  float global_time_offset = rng.get_double (0.0, usr::cycle_duration);
  for (unsigned n = 0; n != count; ++ n) {
    float hue = 6.0f * phase;
    hue = (hue < 3.0f / 2.0f ? hue * 2.0f / 3.0f : hue * 10.0f / 9.0f - 2.0f / 3.0f); // More orange.
    objects [n].hue = hue;

    // Initial animation_time is in [T, 2T) to force an immediate transition.
    float animation_time = global_time_offset + (1.0f - phase) * usr::cycle_duration;
    if (animation_time < usr::cycle_duration) animation_time += usr::cycle_duration;
    objects [n].animation_time = animation_time;
    phase += 1.0f / total_count;
  }

  bumps.initialize (usr::hsv_s_bump, usr::hsv_v_bump);
  step.initialize (usr::morph_start, usr::morph_finish);
  initialize_systems (abc, xyz, xyzinvt, primitive_count, vao_ids);

  return true;
}

#if PRINT_ENABLED
#include <ostream>
#include <iostream>
#include <iomanip>
#endif

model_t::~model_t ()
{
#if PRINT_ENABLED
  std::cout << std::scientific << std::setprecision (8);
  std::cout << "Required range for arccos function: x in [" << min_d << ", " << max_d << "].\n\n";

  double total_polyhedron_count = 0.0;
  for (unsigned n = 0; n != 18; ++ n)
  {
    total_polyhedron_count += polyhedron_counts [n];
  }

  std::cout << std::fixed << std::setprecision (2);
  for (unsigned n = 0; n != 18; ++ n)
  {
    std::cout << std::setw (10) << (100.0 * polyhedron_counts [n] / total_polyhedron_count) << " % " << names [n] << "\n";
  }
#endif
  deallocate (memory);
}

bool model_t::set_capacity (unsigned new_capacity)
{
  return reallocate_aligned_arrays (memory, capacity, new_capacity,
                                    & r, & x, & v, & u, & w,
                                    & kdtree_index,
                                    & objects);
}

void model_t::add_object (v4f view)
{
  if (count == capacity) {
    set_capacity (capacity ? 2 * capacity : 128);
  }

  object_t & A = objects [count];
  float R = rng.get_double (usr::min_radius, usr::max_radius);
  r [count] = R;
  if (max_radius < R) max_radius = R;
  A.m = usr::density * R * R;
  A.l = 0.4f * A.m * R * R;

  float t0 [4] ALIGNED16;
  store4f (t0, view);
  float z1 = t0 [0];
  float z2 = t0 [0] + t0 [1];
  float x1 = t0 [2] / 2;
  float y1 = t0 [3] / 2;
  float x2 = x1 * z2 / z1;
  float y2 = y1 * z2 / z1;
  v4f R0 = { R, 0.0f, 0.0f, 0.0f, };
  v4f a = { -x2 + R, -y2 + R, -z1 - R, 0.0f, };
  v4f b = { +x2 - R, +y2 - R, -z2 + R, 0.0f, };
  v4f m = b - a;
 loop:
  v4f t = a + m * rng.get_vector_in_box ();
  for (unsigned k = 0; k != 6; ++ k) {
    v4f anchor = load4f (walls [k] [0]);
    v4f normal = load4f (walls [k] [1]);
    v4f s = dot (t - anchor, normal);
    if (_mm_comilt_ss (s, R0)) {
      goto loop;
    }
  }
  store4f (x [count], t);
  store4f (v [count], rng.get_vector_in_ball (0.5f * usr::temperature / A.m));
  store4f (u [count], rng.get_vector_in_ball (0x1.921fb4P1)); // pi
  store4f (w [count], rng.get_vector_in_ball (0.2f * usr::temperature / A.l));

  std::uint64_t entropy = rng.get ();
  A.target.system = static_cast <system_select_t> ((entropy >> 3) % 6);
  A.target.point = entropy & 7;
  A.starting_point = A.target.point;
  ++ count;
}

void model_t::recalculate_locus (object_t & object)
{
  system_select_t sselect = object.target.system;
  v4f g0 = load4f (abc [sselect] [object.starting_point]);
  v4f g1 = load4f (abc [sselect] [object.target.point]);
  v4f T0 = tmapply (xyz [sselect], g0);
  v4f T1 = tmapply (xyz [sselect], g1);
  v4f d = dot (T0, T1);
  v4f T2 = normalize (T1 - d * T0);
  v4f g2 = mapply (xyzinvt [sselect], T2);
  store4f (object.locus_end, g2);
  // For allowed transitions we have d in [7.74596691e-001, 9.90879238e-001].
  object.locus_speed = _mm_cvtss_f32 (arccos (d));

#if PRINT_ENABLED
  float df = _mm_cvtss_f32 (d);
  if (min_d > df) min_d = df;
  if (max_d < df) max_d = df;
#endif
}

void model_t::proceed ()
{
  const float dt = usr::frame_time;

  for (unsigned n = 0; n != count; ++ n) {
    object_t & A = objects [n];
    float t = A.animation_time + dt;
    if (t >= usr::cycle_duration) {
      t -= usr::cycle_duration;
      // We must perform a Markov transition.
      transition (rng, u [n], A.target, A.starting_point);
      recalculate_locus (A);
#if PRINT_ENABLED
      ++ polyhedron_counts [polyhedra [(int) A.target.system] [A.target.point] - 1];
#endif
    }
    A.animation_time = t;
  }

  kdtree.compute (kdtree_index, x, count);
  kdtree.bounce (count, 2 * max_radius, objects, r, v, w, walls);
  advance_linear (x, v, count, dt);
  advance_angular (u, w, count, dt);
}

void model_t::draw ()
{
  // Draw all the shapes, one uniform buffer at a time.
  unsigned begin = 0, end = uniform_buffer.count ();
  while (end < count) {
    draw (begin, end - begin);
    begin = end;
    end = begin + uniform_buffer.count ();
  }
  draw (begin, count - begin);
}

void model_t::draw (unsigned begin, unsigned count)
{
  // Set the modelview matrix, m.
  compute (reinterpret_cast <char *> (& uniform_buffer [0].m), uniform_buffer.stride (), & x [begin], & u [begin], count);

  // Set the circumradius, r.
  for (unsigned n = 0; n != count; ++ n) {
    uniform_block_t & block = uniform_buffer [n];
    block.r [0] = r [begin + n];
  }

  // Set the diffuse material reflectance, d.
  for (unsigned n = 0; n != count; ++ n) {
    uniform_block_t & block = uniform_buffer [n];
    const object_t & object = objects [begin + n];
    float saturation, value;
    bumps (object.animation_time, saturation, value);
    store4f (block.d, hsv_to_rgb (object.hue, saturation, value, 0.85f));
  }

  // Set the uniform coefficients, g.
  for (unsigned n = 0; n != count; ++ n) {
    uniform_block_t & block = uniform_buffer [n];
    const object_t & object = objects [begin + n];
    system_select_t sselect = object.target.system;
    {
      v4f t = step (object.animation_time) * _mm_set1_ps (object.locus_speed);
      v4f sc = sincos (t);
      v4f s = _mm_moveldup_ps (sc);
      v4f c = _mm_movehdup_ps (sc);
      v4f g = c * load4f (abc [sselect] [object.starting_point]) + s * load4f (object.locus_end);
      store4f (block.g, g);
    }
  }

  // Precompute triangle altitudes, h (for non-snubs only).
  for (unsigned n = 0; n != count; ++ n) {
    uniform_block_t & block = uniform_buffer [n];
    const object_t & object = objects [begin + n];
    system_select_t sselect = object.target.system;
    unsigned program_select = object.starting_point == 7 || object.target.point == 7 ? 1 : 0;
    if (program_select == 0)
    {
      const float (& X) [3] [4] = xyz [sselect];
      v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
      v4f crs [3], dsq [3];
      for (unsigned i = 0; i != 3; ++ i) {
        v4f Y = load4f (X [(i + 1) % 3]);
        v4f Z = load4f (X [(i + 2) % 3]);
        v4f YZ = dot (Y, Z);
        dsq [i] = YZ * YZ;
        crs [i] = cross (Y, Z);
      }

      v4f tX = dot (load4f (X [0]), crs [0]); // triple product
      v4f UT [3], usq [3];
      for (unsigned i = 0; i != 3; ++ i) {
        v4f a = _mm_set1_ps (block.g [i]);
        UT [i] = (a + a) * tX * crs [i] / (one - dsq [i]);
        usq [i] = dot (UT [i], UT [i]);
      }

      v4f T = tmapply (X, load4f (block.g));

      v4f asq [3], vwsq [3];
      for (unsigned i = 0; i != 3; ++ i) {
        v4f tx = dot (T, load4f (X [i]));
        asq [i] = one - tx * tx;
        v4f vw = dot (UT [(i + 1) % 3], UT [(i + 2) % 3]);
        vwsq [i] = vw * vw;
      }

#define col0(a,b,c,d) _mm_movelh_ps (_mm_unpacklo_ps (a, b), _mm_unpacklo_ps (c, d))

      v4f q = { 0.25f, 0.25f, 0.25f, 0.25f, };
      for (unsigned i = 0; i != 3; ++ i) {
        unsigned j = (i + 1) % 3;
        unsigned k = (i + 2) % 3;
        v4f H = sqrt (col0 (asq [i] - q * usq [j], asq [i] - q * usq [k], usq [k] - vwsq [i] * rcp (usq [j]), usq [j] - vwsq [i] * rcp (usq [k])));
        store4f (block.h [i], H);
      }
    }
  }

  uniform_buffer.update ();

  for (unsigned n = 0; n != count; ++ n) {
    const object_t & object = objects [begin + n];
    system_select_t sselect = object.target.system;
    unsigned program_select = object.starting_point == 7 || object.target.point == 7 ? 1 : 0;

    paint (primitive_count [sselect],
           vao_ids [sselect],
           programs [program_select],
           uniform_buffer.id (),
           n * uniform_buffer.stride ());
  }
}
