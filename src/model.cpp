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
  float t [4];
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
  for (unsigned n = 0; n != usr::count; ++ n) add_object (static_cast <float> (n) / usr::count, view);
  for (unsigned n = 0; n != count; ++ n) zorder_index [n] = n;
  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;
  quicksort (zorder_index, x, count);

  animation_time_lo = 0.0f;
  animation_time_hi = 0;

  bumps.initialize (0.25f, 1.25f, 0.35f, 0.50f, 4.50f, 6.00f,
                    0.00f, 1.00f, 0.50f, 0.65f, 5.50f, 6.50f);
  step.initialize (usr::morph_start, usr::morph_finish);

  initialize_systems (abc, xyz, primitive_count, vao_ids);

  return true;
}

model_t::~model_t ()
{
  deallocate (memory);
}

void model_t::set_capacity (unsigned new_capacity)
{
  reallocate_aligned_arrays (memory, capacity, new_capacity,
                             & x, & v, & u, & w,
                             & zorder_index, & kdtree_index,
                             & objects);
}

void model_t::add_object (float phase, v4f view)
{
  if (count == capacity) {
    set_capacity (capacity ? 2 * capacity : 128);
  }

  object_t & A = objects [count];
  A.r = rng.get_double (1.67f, 1.67f);
  if (max_radius < A.r) max_radius = A.r;
  A.m = usr::mass * A.r * A.r;
  A.l = 0.4f * A.m * A.r * A.r;
  A.hue = 6.0f * phase;
  A.generator_position = 0.0f;
  A.locus_begin = rng.get () & 7;
  A.locus_end = markov (rng, A.locus_begin, A.locus_begin);
  A.system_select = static_cast <system_select_t> (3.0f * phase);
  A.program_select = (A.locus_begin == 7 || A.locus_begin == 7) ? (rng.get () & 1) + 1 : 0;

  float t0 [4];
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

    float temp [4];
    store4f (temp, bumps (t));
    A.value = temp [0];
    A.saturation = temp [1];

    if (t < step.T [0] && A.generator_position) {
      // We must perform a Markov transition.
      unsigned next = markov (rng, A.locus_end, A.locus_begin);
      A.locus_begin = A.locus_end;
      A.locus_end = next;
      // Choose the shader program (ordinary, snub or antisnub).
      A.program_select =
        A.locus_begin == 7 ? A.program_select :
        A.locus_end == 7 ? (rng.get () & 1) + 1 :
        0;
    }
    A.generator_position = step (t);
  }

  // Use insertion sort on the assumption that the z-order
  // hasn't changed much since the last frame.
  insertion_sort (zorder_index, x, 0, count);
}

namespace
{
  inline v4f hsv_to_rgb0 (float h, float s, float v)
  {
    const v4f num [] ALIGNED16 = {
      { 0.0f, 0.0f, 0.0f, 0.0f, },
      { 1.0f, 1.0f, 1.0f, 0.0f, },
      { 2.0f, 2.0f, 2.0f, 0.0f, },
      { 3.0f, 3.0f, 3.0f, 0.0f, },
      { 4.0f, 4.0f, 4.0f, 0.0f, },
      { 5.0f, 5.0f, 5.0f, 0.0f, },
      { 6.0f, 6.0f, 6.0f, 0.0f, },
    };
    v4f theta = { h, h + 4.0f, h + 2.0f, 0.0f, };
    theta -= _mm_and_ps (_mm_cmpge_ps (theta, num [6]), num [6]);
    v4f v4 = _mm_set1_ps (v);
    v4f s4 = _mm_set1_ps (s);
    v4f lt2 = _mm_cmplt_ps (theta, num [2]);
    v4f lt3 = _mm_cmplt_ps (theta, num [3]);
    v4f lt5 = _mm_cmplt_ps (theta, num [5]);
    v4f m23 = _mm_andnot_ps (lt2, lt3);
    v4f f_lt2 = _mm_and_ps (lt2, num [1]);
    v4f f_m23 = _mm_and_ps (m23, num [3] - theta);
    v4f f_ge5 = _mm_andnot_ps (lt5, theta - num [5]);
    v4f f = _mm_or_ps (_mm_or_ps (f_lt2, f_m23), f_ge5);
    return v4 * (num [1] - s4) + v4 * s4 * f;
  }
}

void model_t::draw ()
{
  float f [16] ALIGNED16;
  float rgb0 [4] ALIGNED16;
  float abc0 [4] ALIGNED16;

  for (unsigned n = 0; n != count; ++ n) {
    unsigned nz = zorder_index [n];
    compute (f, x [nz], u [nz]);

    const object_t & object = objects [nz];
    store4f (rgb0, hsv_to_rgb0 (object.hue, object.saturation, object.value));

    unsigned sselect = object.system_select;
    {
      v4f alpha = _mm_set1_ps (object.generator_position);
      v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
      v4f A = load4f (abc [sselect] [object.locus_begin]);
      v4f B = load4f (abc [sselect] [object.locus_end]);
      v4f g = (one - alpha) * A + alpha * B;
      v4f a, b, c;
      UNPACK3 (g, a, b, c);
      v4f t = a * load4f (xyz [sselect] [0]) + b * load4f (xyz [sselect] [1]) + c * load4f (xyz [sselect] [2]);
      store4f (abc0, g * _mm_rsqrt_ps (dot (t, t)));
    }

    paint (object.r, f, rgb0, abc0,
           primitive_count [sselect], vao_ids [sselect],
           programs [object.program_select]);
  }
}
