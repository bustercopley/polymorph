#include "kdtree.h"
#include "model.h"
#include "object.h"
#include "frustum.h"
#include "random.h"
#include "bounce.h"
#include "markov.h"
#include "random.h"
#include "partition.h"
#include "graphics.h"
#include "config.h"
#include "maths.h"
#include "memory.h"
#include "aligned-arrays.h"

#include <type_traits>
static_assert (std::is_trivial <object_t>::value, "non-trivial type object_t will be allocated in raw memory");

void model_t::initialize (void * data, unsigned long long seed, const view_t & frustum)
{
  repository.initialize (data);
  rng.initialize (seed);
  memory = nullptr;
  walls_memory = nullptr;
  count = 0;
  capacity = 0;
  max_radius = 0;
  animation_time = usr::cycle_duration;
  view = frustum;
  float z1 = -view.distance;
  float z2 = -view.distance - view.depth;
  float x1 = view.width / 2;
  float y1 = view.height / 2;
  float x2 = x1 * z2 / z1;
  float y2 = y1 * z2 / z1;

  const float temp [6] [2] [4] = {
    { { 0.0f, 0.0f, z1, 0.0f, }, { 0.0f, 0.0f, -1.0f, 0.0f, }, },
    { { 0.0f, 0.0f, z2, 0.0f, }, { 0.0f, 0.0f,  1.0f, 0.0f, }, },
    { {  -x1, 0.0f, z1, 0.0f, }, { z1 - z2, 0.0f, x1 - x2, 0.0f, }, },
    { {   x1, 0.0f, z1, 0.0f, }, { z2 - z1, 0.0f, x1 - x2, 0.0f, }, },
    { { 0.0f,  -y1, z1, 0.0f, }, { 0.0f, z1 - z2, y1 - y2, 0.0f, }, },
    { { 0.0f,   y1, z1, 0.0f, }, { 0.0f, z2 - z1, y1 - y2, 0.0f, }, },
  };

  unsigned dummy = 0;
  reallocate_aligned_arrays (walls_memory, dummy, 6, & walls);
  for (unsigned k = 0; k != 6; ++ k) {
    __m128 anchor = _mm_load_ps (temp [k] [0]);
    __m128 normal = _mm_load_ps (temp [k] [1]);
    __m128 nlen = _mm_sqrt_ps (dot (normal, normal));
    _mm_store_ps (walls [k] [0], anchor);
    _mm_store_ps (walls [k] [1], _mm_div_ps (normal, nlen));
  }

  set_capacity (usr::count);
  for (unsigned n = 0; n != usr::count; ++ n) add_object (static_cast <float> (n) / usr::count);
  for (unsigned n = 0; n != count; ++ n) zorder_index [n] = n;
  for (unsigned n = 0; n != count; ++ n) kdtree_index [n] = n;
  quicksort (zorder_index, x, count);
}

model_t::~model_t ()
{
  deallocate (memory);
  deallocate (walls_memory);
}

void model_t::set_capacity (unsigned new_capacity)
{
  reallocate_aligned_arrays (memory, capacity, new_capacity,
                             & x, & v, & u, & w,
                             & zorder_index, & kdtree_index,
                             & objects);
}

void model_t::add_object (float phase)
{
  if (count == capacity)
    set_capacity (capacity ? 2 * capacity : 128);

  object_t & A = objects [count];
  A.r = rng.get_double (1.0, 2.0);
  A.m = usr::mass * A.r * A.r;
  A.l = 0.4f * A.m * A.r * A.r;

  A.start (rng);
  A.system_ref = repository.ref (static_cast <system_select_t> (3.0f * phase));
  A.hue = 6.0f * phase;

  float z1 = view.distance;
  float z2 = view.distance + view.depth;
  float x1 = view.width;
  float y1 = view.height;
  float x2 = x1 * z2 / z1;
  float y2 = y1 * z2 / z1;
  __m128 a = { -x2 + A.r, -y2 + A.r, -z1 - A.r, 0.0f, };
  __m128 b = { +x2 - A.r, +y2 - A.r, -z2 + A.r, 0.0f, };
  __m128 m = _mm_sub_ps (b, a);
  __m128 rsq = _mm_set1_ps (A.r * A.r);
loop:
  __m128 t = _mm_add_ps (a, _mm_mul_ps (m, rng.get_vector_in_box ()));
  for (unsigned k = 0; k != 6; ++ k) {
    __m128 anchor = _mm_load_ps (walls [k] [0]);
    __m128 normal = _mm_load_ps (walls [k] [1]);
    __m128 ssq = dot (_mm_sub_ps (t, anchor), normal);
    if (_mm_comilt_ss (ssq, rsq)) {
      goto loop;
    }
  }
  __m128 temp = rng.get_vector_in_ball (pi);
  __m128d ulo = _mm_cvtps_pd (temp);
  __m128d uhi = _mm_cvtps_pd (_mm_movehl_ps (temp, temp));
  _mm_store_pd (& u [count] [0], ulo);
  _mm_store_pd (& u [count] [2], uhi);
  _mm_store_ps (w [count], rng.get_vector_in_ball (0.2 * usr::temperature / A.l));
  _mm_store_ps (x [count], t);
  _mm_store_ps (v [count], rng.get_vector_in_ball (0.5 * usr::temperature / A.m));

  if (max_radius < A.r) max_radius = A.r;
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

void model_t::proceed (real dt)
{
  animation_time += dt;
  if (usr::cycle_duration <= animation_time)
    animation_time -= usr::cycle_duration;

  real T = usr::cycle_duration;
  real TN = T / count;
  real t = animation_time;
  unsigned d = 1 + static_cast <unsigned> (t / TN);
  real s = t + T - d * TN;

  kdtree.compute (kdtree_index, x, count);
  kdtree.for_near (count, 2 * max_radius, this, ball_bounce_callback);
  kdtree.for_near (walls, max_radius, this, wall_bounce_callback);

  advance_linear (x, v, count, dt);
  advance_angular (u, w, count, dt);

  for (unsigned n = 0; n != count; ++ n) {
    objects [(d + n) % count].update_appearance (s - n * TN, rng);
  }

  // Use insertion sort on the assumption that the z-order
  // hasn't changed much since the last frame.
  insertion_sort (zorder_index, x, 0, count - 1);
}

void model_t::draw ()
{
  for (unsigned n = 0; n != count; ++ n) {
    unsigned nz = zorder_index [n];
    float f [16];
    compute (f, x [nz], u [nz], objects [nz].r);
    paint (objects [nz], f);
  }
}
