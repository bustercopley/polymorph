#include "compiler.h"
#include "memory.h"
#include "kdtree.h"
#include "model.h"
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

#ifdef PRINT_ENABLED
// These are Father Wenninger's numbers.
unsigned polyhedra [system_count] [8] = {
 {  2,  1,  1, 11,  6,  6,  7,  4, },
 {  2,  1,  1, 11,  6,  6,  7,  4, },
 { 11,  3,  2, 13,  7,  8, 15, 17, },
 { 11,  3,  2, 13,  7,  8, 15, 17, },
 { 12,  5,  4, 14,  9, 10, 16, 18, },
 { 12,  5,  4, 14,  9, 10, 16, 18, },
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
#endif

#ifdef ENABLE_TEST_MODE
#define TEST_MODE_ENABLED 1
#else
#define TEST_MODE_ENABLED 0
#endif

bool model_t::initialize (unsigned long long seed, int width, int height)
{
  rng.initialize (seed);

#if 0
  float tz = usr::tank_distance, td = usr::tank_depth, th = usr::tank_height;
  v4f view = { tz, td, (th * width) / height, th, };
#else
  typedef std::int32_t v4i __attribute__ ((vector_size (16)));
  v4i wi = { height, height, width, height, };
  v4f hw = _mm_cvtepi32_ps ((__m128i) wi);
  v4f hh = _mm_movelh_ps (hw, hw);
  v4f ratio = hw / hh;
  v4f vw = { usr::tank_distance, usr::tank_depth, usr::tank_height, usr::tank_height, };
  v4f view = _mm_mul_ps (vw, ratio);
#endif

  if (! initialize_programs (programs, view)) return false;

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

  unsigned total_count = usr::fill_ratio * width / height;

  count = 0;
  set_capacity (total_count);
  max_radius = 0.0f;

  if (TEST_MODE_ENABLED) {
    float rx = rng.get_double (-1.0, 1.0);
    float ry = rng.get_double (-1.0, 1.0);
    float rz = rng.get_double (-1.0, 1.0);
    float random_rotation [3] = { rx, ry, rz, };

    struct replacement_t
    {
      polyhedron_select_t before, after;
      float rotation [3];
      unsigned probability;
    };

    const float pi = 0x1.921fb4P1f;

    static const replacement_t replacements [] =
    {
      { { tetrahedral, 0, }, { dual_tetrahedral, 0, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 1, }, { dual_tetrahedral, 1, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 2, }, { dual_tetrahedral, 2, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 3, }, { dual_tetrahedral, 3, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 4, }, { dual_tetrahedral, 4, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 5, }, { dual_tetrahedral, 5, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 6, }, { dual_tetrahedral, 6, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 7, }, { dual_tetrahedral, 7, }, { 0, 0, 0, }, 48, },

      { { octahedral, 0, }, { dual_octahedral, 0, }, { 0, 0, 0, }, 48, },
      { { octahedral, 1, }, { dual_octahedral, 1, }, { 0, 0, 0, }, 48, },
      { { octahedral, 2, }, { dual_octahedral, 2, }, { 0, 0, 0, }, 48, },
      { { octahedral, 3, }, { dual_octahedral, 3, }, { 0, 0, 0, }, 48, },
      { { octahedral, 4, }, { dual_octahedral, 4, }, { 0, 0, 0, }, 48, },
      { { octahedral, 5, }, { dual_octahedral, 5, }, { 0, 0, 0, }, 48, },
      { { octahedral, 6, }, { dual_octahedral, 6, }, { 0, 0, 0, }, 48, },
      { { octahedral, 7, }, { dual_octahedral, 7, }, { 0, 0, 0, }, 48, },

      { { icosahedral, 0, }, { dual_icosahedral, 0, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 1, }, { dual_icosahedral, 1, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 2, }, { dual_icosahedral, 2, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 3, }, { dual_icosahedral, 3, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 4, }, { dual_icosahedral, 4, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 5, }, { dual_icosahedral, 5, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 6, }, { dual_icosahedral, 6, }, { 0, 0, 0, }, 48, },
      { { icosahedral, 7, }, { dual_icosahedral, 7, }, { 0, 0, 0, }, 48, },

      { { tetrahedral, 0, }, { octahedral,  1, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 6, }, { octahedral,  5, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 3, }, { octahedral,  0, }, { 0, 0, 0, }, 48, },
      { { tetrahedral, 7, }, { icosahedral, 1, }, { + pi / 4, 0, 0, }, 72, },
      { { tetrahedral, 7, }, { dual_tetrahedral, 7, }, { pi / 2, 0, 0, }, 64, }, // snub, but not chiral
    };

    static const unsigned replacement_count = sizeof replacements / sizeof * replacements;

    for (unsigned n = 0; n != 2 * replacement_count; ++ n) {
      add_object (view);
      object_t & A = objects [count - 1];

      float t0 [4] ALIGNED16;
      store4f (t0, view);
      float z1 = t0 [0];
      float z2 = t0 [0] + t0 [1];

      x [n] [0] = -12.0f + (n / 16) * 6.0f + (n % 2) * 2.0f;
      x [n] [1] = +8.0f - ((n % 16) / 2) * 2.0f;
      x [n] [2] = -0.5f * (z1 + z2);

      v4f zero = _mm_setzero_ps ();
      store4f (u [n], zero);
      store4f (v [n], zero);
      store4f (w [n], zero);

      const replacement_t & replacement = replacements [n / 2];
      polyhedron_select_t target;
      target = (n % 2) ? replacement.after : replacement.before;

      A.hue = ((target.system & ~1) == 0 ? 1 : (target.system & ~1) == octahedral ? 3 : 5) - (target.system & 1);

      if (1) {
        w [n] [0] = +0.00;
        w [n] [1] = +0.00;
        w [n] [2] = +0.00;
      }

      if (0) {
        rotate (u [n], random_rotation);
      }

      if (n % 2) rotate (u [n], replacement.rotation);
      A.starting_point = target.point;
      A.target = target;
    }
  }
  else {
    for (unsigned n = 0; n != total_count; ++ n) add_object (view);
  }

  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;

  animation_time = 0.0f;
  bumps.initialize (usr::hsv_s_bump, usr::hsv_v_bump);
  step.initialize (usr::morph_start, usr::morph_finish);
  initialize_systems (abc, xyz, primitive_count, vao_ids);
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
  double total_polyhedron_count = 0.0;
  for (unsigned n = 0; n != 18; ++ n)
  {
    total_polyhedron_count += polyhedron_counts [n];
  }

  std::cout << std::fixed << std::setprecision (1);
  for (unsigned n = 0; n != 18; ++ n)
  {
    std::cout << std::setw (10) << (100.0 * polyhedron_counts [n] / total_polyhedron_count) << " % " << names [n] << "\n";
  }
  std::cout << std::flush;
#endif
  deallocate (memory);
}

void model_t::set_capacity (unsigned new_capacity)
{
  reallocate_aligned_arrays (memory, capacity, new_capacity,
                             & r, & x, & v, & u, & w, & f, & g, & d,
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
  A.phase = rng.get_double (0.0, 1.0);
  A.hue = 6.0f * (1.0f - A.phase);
  A.generator_position = 0.0f;
  A.target.system = static_cast <system_select_t> ((entropy >> 3) % 6);
  A.target.point = entropy & 7;
  A.starting_point = A.target.point;
  transition (rng, u [count], A.target, A.starting_point);
  ++ count;
}

void model_t::proceed ()
{
  const float dt = usr::frame_time;
  const float T = usr::cycle_duration;
  float value, saturation;
  animation_time += dt;
  if (animation_time >= T) animation_time -= T;

  for (unsigned n = 0; n != count; ++ n) {
    object_t & A = objects [n];
    float t = animation_time + A.phase * T;
    if (t >= T) t -= T;
    if (TEST_MODE_ENABLED) t = usr::hsv_v_bump.t2;
    bumps (t, saturation, value);
    store4f (d [n], hsv_to_rgb (A.hue, saturation, value, 0.85f));
    if (t < step.T [0] && A.generator_position) {
      // We must perform a Markov transition.
      transition (rng, u [n], A.target, A.starting_point);
#if PRINT_ENABLED
      ++ polyhedron_counts [polyhedra [(int) A.target.system] [A.target.point] - 1];
#endif
    }
    A.generator_position = step (t);
  }

  kdtree.compute (kdtree_index, x, count);
  kdtree.bounce (count, 2 * max_radius, objects, r, v, w, walls);
  advance_linear (x, v, count, dt);
  advance_angular (u, w, count, dt);
  compute (f, x, u, count);

  for (unsigned n = 0; n != count; ++ n) {
    object_t & object = objects [n];
    unsigned sselect = object.target.system;
    v4f alpha = _mm_set1_ps (object.generator_position);
    v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
    v4f A = load4f (abc [sselect] [object.starting_point]);
    v4f B = load4f (abc [sselect] [object.target.point]);
    v4f k = (one - alpha) * A + alpha * B;
    v4f a, b, c;
    UNPACK3 (k, a, b, c);
    v4f t = a * load4f (xyz [sselect] [0]) + b * load4f (xyz [sselect] [1]) + c * load4f (xyz [sselect] [2]);
    store4f (g [n], k * _mm_rsqrt_ps (dot (t, t)));
  }
}

void model_t::draw ()
{
  for (unsigned n = 0; n != count; ++ n) {
    const object_t & object = objects [n];
    system_select_t system_select = object.target.system;
    unsigned program_select = object.starting_point == 7 || object.target.point == 7 ? 1 : 0;
    paint (r [n], f [n], g [n], d [n],
           primitive_count [system_select],
           vao_ids [system_select],
           programs [program_select]);
  }
}
