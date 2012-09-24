#include "kdtree.h"
#include "model.h"
#include "model.h"
#include "random.h"
#include "rodrigues.h"
#include "bounce.h"
#include "markov.h"
#include "random.h"
#include "partition.h"
#include "graphics.h"
#include "config.h"
#include "memory.h"
#include "aligned-arrays.h"
#include "vector.h"
#include "compiler.h"
#include <cmath>

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

  max_radius = 0;
  animation_time = usr::cycle_duration;

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
  for (unsigned n = 0; n != usr::count; ++ n) add_object (static_cast <float> (n) / usr::count, view);
  for (unsigned n = 0; n != count; ++ n) zorder_index [n] = n;
  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;
  quicksort (zorder_index, x, count);

  initialize_systems (abc, xyz, primitive_count, vao_ids);

  union {
    std::uint32_t u32 [3] [4];
    v4f f32 [3];
  } tmask = {
    {
      { 0xffffffff, 0, 0, 0, },
      { 0, 0xffffffff, 0, 0, },
      { 0, 0, 0xffffffff, 0, },
    }
  };

  for (unsigned i = 0; i != 3; ++ i) {
    store4f (masks [2 * i] [0], tmask.f32 [i]);
    store4f (masks [2 * i + 1] [0], tmask.f32 [i]);
    store4f (masks [2 - i] [1], tmask.f32 [i]);
    store4f (masks [5 - i] [1], tmask.f32 [i]);
  }

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
  A.r = rng.get_double (2.0, 2.0);
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

void ball_bounce_callback (void * data, unsigned i, unsigned j)
{
  reinterpret_cast <model_t *> (data)->ball_bounce (i, j);
}

void wall_bounce_callback (void * data, unsigned i, unsigned j)
{
  reinterpret_cast <model_t *> (data)->wall_bounce (i, j);
}

void model_t::ball_bounce (unsigned i, unsigned j)
{
  ::bounce (i, j, objects, x, v, w);
}

void model_t::wall_bounce (unsigned i, unsigned j)
{
  ::bounce (walls [i], j, objects, x, v, w);
}

void model_t::proceed ()
{
  kdtree.compute (kdtree_index, x, count);
  kdtree.for_near (count, 2 * max_radius, this, ball_bounce_callback);
  kdtree.for_near (walls, max_radius, this, wall_bounce_callback);

  advance_linear (x, v, count, usr::frame_time);
  advance_angular (u, w, count, usr::frame_time);

  const float T = usr::cycle_duration;
  const float TN = T / count;
  animation_time = std::fmod (animation_time + usr::frame_time, T);
  float s = T + std::fmod (animation_time, TN);
  unsigned d = static_cast <unsigned> (animation_time / TN);

  for (unsigned n = 0; n != count; ++ n) {
    object_t & A = objects [(d + n) % count];
    float t = s - n * TN;
    A.value = usr::value_bump (t);
    A.saturation = usr::saturation_bump (t);

    // Adjust `locus_begin', `locus_end' and `generator_position',
    // the parameters used in the function `paint_polyhedron'
    // in "graphics.cpp" to locate a point on the Moebius triangle.

    const float T0 = usr::morph_start;
    const float T1 = usr::morph_finish;

    if (t < T0) {
      if (A.generator_position) {
        A.generator_position = 0.0f;
        // We must perform a Markov transition.
        unsigned next = markov (rng, A.locus_end, A.locus_begin);
        A.locus_begin = A.locus_end;
        A.locus_end = next;
        A.program_select =
          A.locus_begin == 7 ? A.program_select :
          A.locus_end == 7 ? (rng.get () & 1) + 1 :
          0;
      }
    }
    else if (t < T1) {
      float s = (t - T0) / (T1 - T0);
      A.generator_position = s * s * (3 - 2 * s);
    }
    else {
      A.generator_position = 1.0f;
    }
  }

  // Use insertion sort on the assumption that the z-order
  // hasn't changed much since the last frame.
  insertion_sort (zorder_index, x, 0, count);
}

inline v4f hsv_to_rgb0 (float h, float s, float v, float (& masks) [6] [2] [4])
{

  v4f ss = _mm_set1_ps (s);
  v4f vv = _mm_set1_ps (v);
  v4f cc = vv * ss;
  v4f mm = vv - cc;
  v4f xx = cc * _mm_set1_ps (std::abs (std::fmod (h, 2.0f) - 1.0f));
  unsigned ih = static_cast <unsigned> (h);
  v4f cmask = load4f (masks [ih] [0]);
  v4f xmask = load4f (masks [ih] [1]);

  return mm + _mm_or_ps (_mm_and_ps (cc, cmask), _mm_and_ps (xx, xmask));
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
    store4f (rgb0, hsv_to_rgb0 (object.hue, object.saturation, object.value, masks));

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
