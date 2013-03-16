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

// These are Father Wenninger's numbers.
unsigned polyhedra [3] [8] = {
 {  2,  1,  1, 11,  6,  6,  7,  4, },
 { 11,  3,  2, 13,  7,  8, 15, 17, },
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

bool model_t::initialize (unsigned long long seed, int width, int height)
{
  rng.initialize (seed);

#if 0
  float td = usr::tank_distance, tz = usr::tank_depth, th = usr::tank_height;
  v4f view = { td, tz, (th * width) / height, th, };
#else
  typedef std::int32_t v4i __attribute__ ((vector_size (16)));
  v4i wi = { height, height, width, height, };
  v4f hw = _mm_cvtepi32_ps ((__m128i) wi);
  v4f hh = _mm_movelh_ps (hw, hw);
  v4f ratio = hw / hh;
  v4f v = { usr::tank_distance, usr::tank_depth, usr::tank_height, usr::tank_height, };
  v4f view = _mm_mul_ps (v, ratio);
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
  for (unsigned n = 0; n != total_count; ++ n) add_object (view);
  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;

  animation_time = 0.0f;
  bumps.initialize (usr::hsv_v_bump, usr::hsv_s_bump);
  step.initialize (usr::morph_start, usr::morph_finish);
  initialize_systems (abc, xyz, primitive_count, vao_ids);
  return true;
}

#include <ostream>
#include <iostream>
#include <iomanip>

model_t::~model_t ()
{
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
  v4f r = { R, 0.0f, 0.0f, 0.0f, };
  v4f a = { -x2 + R, -y2 + R, -z1 - R, 0.0f, };
  v4f b = { +x2 - R, +y2 - R, -z2 + R, 0.0f, };
  v4f m = b - a;
 loop:
  v4f t = a + m * rng.get_vector_in_box ();
  for (unsigned k = 0; k != 6; ++ k) {
    v4f anchor = load4f (walls [k] [0]);
    v4f normal = load4f (walls [k] [1]);
    v4f s = dot (t - anchor, normal);
    if (_mm_comilt_ss (s, r)) {
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
  A.target.system = static_cast <system_select_t> ((entropy >> 5) % 3);
  A.target.point = (entropy >> 1) & 7;
  A.target.program = (A.target.point == 7) ? (entropy & 1) + 1 : 0;
  A.starting_point = A.target.point;
  transition (rng, u [count], A.target, A.starting_point);
  ++ count;
}

void model_t::proceed ()
{
  const float dt = usr::frame_time;
  const float T = usr::cycle_duration;
  animation_time += dt;
  if (animation_time >= T) animation_time -= T;

  for (unsigned n = 0; n != count; ++ n) {
    object_t & A = objects [n];
    float t = animation_time + A.phase * T;
    if (t >= T) t -= T;
    bumps (t, A.value, A.saturation);
    if (t < step.T [0] && A.generator_position) {
      // We must perform a Markov transition.
      transition (rng, u [n], A.target, A.starting_point);
      ++ polyhedron_counts [polyhedra [(int) A.target.system] [A.target.point] - 1];
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
    store4f (d [n], hsv_to_rgb (object.hue, object.saturation, object.value, 0.85f));
  }

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
    unsigned program_select = object.target.program;
    paint (r [n], f [n], g [n], d [n],
           primitive_count [system_select],
           vao_ids [system_select],
           programs [program_select]);
  }
}
