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
#include "memory.h"
#include "aligned-arrays.h"
#include "vector.h"
#include "compiler.h"

// These are Father Wenninger's numbers.
unsigned polyhedra [3] [8] =
{
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

  count = 0;
  set_capacity (usr::count);
  max_radius = 0.0f;
  for (unsigned n = 0; n != usr::count; ++ n) add_object (view, usr::count);
  for (unsigned n = 0; n != count; ++ n) zorder_index [n] = n;
  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;
  quicksort (zorder_index, x, count);

  animation_time_lo = 0.0f;
  animation_time_hi = 0;

  bumps.initialize (0.15f, 0.75f, 0.25f, 0.50f, 2.00f, 2.50f,  // HSV value bump.
                    0.00f, 1.00f, 0.25f, 0.50f, 2.00f, 2.50f); // HSV saturation bump.
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
                             & x, & v, & u, & w,
                             & zorder_index, & kdtree_index,
                             & objects);
}

void model_t::add_object (v4f view, unsigned total_count)
{
  if (count == capacity) {
    set_capacity (capacity ? 2 * capacity : 128);
  }

  object_t & A = objects [count];
  A.r = 1.0f;
  if (max_radius < A.r) max_radius = A.r;
  A.m = usr::mass * A.r * A.r;
  A.l = 0.4f * A.m * A.r * A.r;

  float t0 [4] ALIGNED16;
  store4f (t0, view);
  float z1 = t0 [0];
  float z2 = t0 [0] + t0 [1];
  float x1 = t0 [2] / 2;
  float y1 = t0 [3] / 2;
  float x2 = x1 * z2 / z1;
  float y2 = y1 * z2 / z1;
  v4f r = { A.r, 0.0f, 0.0f, 0.0f, };
  v4f a = { -x2 + A.r, -y2 + A.r, -z1 - A.r, 0.0f, };
  v4f b = { +x2 - A.r, +y2 - A.r, -z2 + A.r, 0.0f, };
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

  A.hue = count * 6.0f / total_count;
  A.generator_position = 0.0f;
  A.target.system = tetrahedral; //static_cast <system_select_t> (3.0f * phase);
  A.target.point = rng.get () & 7;
  A.target.program = (A.target.point == 7) ? (rng.get () & 1) + 1 : 0;
  A.starting_point = A.target.point;
  transition (rng, u [count], A.target, A.starting_point);

  ++ count;
}

void model_t::proceed ()
{
  kdtree.compute (kdtree_index, x, count);
  kdtree.bounce (count, 2 * max_radius, objects, v, w, walls);

  const float T = usr::cycle_duration;
  const float dt = usr::frame_time;

  advance_linear (x, v, count, dt);
  advance_angular (u, w, count, dt);

  animation_time_lo += dt;
  const float TN = T / count;
  while (animation_time_lo >= TN) {
    animation_time_lo -= TN;
    ++ animation_time_hi;
  }
  if (animation_time_hi >= count) {
    animation_time_hi -= count;
  }

  unsigned k = animation_time_hi;
  for (unsigned n = 0; n != count; ++ n) {
    object_t & A = objects [n];
    float t = k * TN + animation_time_lo;
    if (! k) k = count;
    -- k;

    float temp [4] ALIGNED16;
    store4f (temp, bumps (t));
    A.value = temp [0];
    A.saturation = temp [1];

    if (t < step.T [0] && A.generator_position)
    {
      // We must perform a Markov transition.
      transition (rng, u [n], A.target, A.starting_point);
      ++ polyhedron_counts [polyhedra [(int) A.target.system] [A.target.point] - 1];
    }
    A.generator_position = step (t);
  }

  // Use insertion sort on the assumption that the z-order
  // hasn't changed much since the last frame.
  insertion_sort (zorder_index, x, 0, count);
}

namespace
{
  inline v4f hsva_to_rgba (float hue, float saturation, float value, float alpha)
  {
    const v4f num1 = { 1.0f, 1.0f, 1.0f, 1.0f, };
    const v4f num2 = { 2.0f, 2.0f, 2.0f, 2.0f, };
    const v4f num3 = { 3.0f, 3.0f, 3.0f, 3.0f, };
    const v4f num5 = { 5.0f, 5.0f, 5.0f, 5.0f, };
    const v4f num6 = { 6.0f, 6.0f, 6.0f, 6.0f, };

    // Use offsets (0,4,2,*) to get [magenta, red, yellow, green, cyan, blue, magenta).
    const v4f offsets = { 0.0f, 4.0f, 2.0f, 0.0f, };
    v4f theta = _mm_set1_ps (hue) + offsets;

    // Reduce each component of theta modulo 6.0f.
    theta -= _mm_and_ps (_mm_cmpge_ps (theta, num6), num6);

    // Apply a function to each component of theta:

    // f[i] ^
    //      |
    //    1 +XXXXXXXX---+---+---+---X--
    //      |        X             X
    //      |         X           X
    //      |          X         X
    //    0 +---+---+---XXXXXXXXX---+--> theta[i]
    //      0   1   2   3   4   5   6

    // f [i] = 1 if theta [i] < 2,
    // f [i] = 3 - theta [i] if 2 <= theta [i] < 3,
    // f [i] = theta [i] - 5 if 5 <= theta [i],
    // f [i] = 0 otherwise.

    // The value of f [3] is unused.

    v4f lt2 = _mm_cmplt_ps (theta, num2);
    v4f lt3 = _mm_cmplt_ps (theta, num3);
    v4f m23 = _mm_andnot_ps (lt2, lt3);
    v4f ge5 = _mm_cmpge_ps (theta, num5);

    v4f term1 = _mm_and_ps (lt2, num1);
    v4f term2 = _mm_and_ps (m23, num3 - theta);
    v4f term3 = _mm_and_ps (ge5, theta - num5);

    v4f f = _mm_or_ps (_mm_or_ps (term1, term2), term3);

    v4f va = { value, value, value, alpha, };
    v4f s0 = { saturation, saturation, saturation, 0.0f, };
    v4f chroma = va * s0;     // x x x 0
    v4f base = va - chroma;   // y y y a
    return base + chroma * f; // r g b a
  }
}

void model_t::draw ()
{
  float f [16] ALIGNED16;
  float rgba [4] ALIGNED16;
  float abc0 [4] ALIGNED16;

  for (unsigned n = 0; n != count; ++ n) {
    unsigned nz = zorder_index [n];
    compute (f, x [nz], u [nz]);

    const object_t & object = objects [nz];
    store4f (rgba, hsva_to_rgba (object.hue, object.saturation, object.value, 0.85f));

    unsigned sselect = object.target.system;
    {
      v4f alpha = _mm_set1_ps (object.generator_position);
      v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
      v4f A = load4f (abc [sselect] [object.starting_point]);
      v4f B = load4f (abc [sselect] [object.target.point]);
      v4f g = (one - alpha) * A + alpha * B;
      v4f a, b, c;
      UNPACK3 (g, a, b, c);
      v4f t = a * load4f (xyz [sselect] [0]) + b * load4f (xyz [sselect] [1]) + c * load4f (xyz [sselect] [2]);
      store4f (abc0, g * _mm_rsqrt_ps (dot (t, t)));
    }

    paint (object.r, f, rgba, abc0,
           primitive_count [sselect], vao_ids [sselect],
           programs [object.target.program]);
  }
}
