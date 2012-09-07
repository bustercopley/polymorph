#include "mswin.h"
#include "kdtree.h"
#include "partition.h"
#include "memory.h"
#include "aligned-arrays.h"
#include "vector.h"

namespace
{
  unsigned nodes_required (unsigned point_count)
  {
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    unsigned m = 2;
    unsigned t = point_count - 1;
    while (t >>= 1) m <<= 1; // Now m is 2^{ceil(log_2(count))}.
    return m - 1;
  }

  unsigned node_dimension (unsigned i)
  {
    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    unsigned m = 0;
    unsigned t = i + 1;
    while (t >>= 1) ++ m; // Now m is floor(log_2(i + 1)).
    return m % 3;
  }
}

kdtree_t::~kdtree_t ()
{
  deallocate (memory);
}

#define SETNODE(_i, _lo, _hi, _begin, _end)  \
  do {                                       \
    _mm_store_ps (node_lohi [_i] [0], _lo);  \
    _mm_store_ps (node_lohi [_i] [1], _hi);  \
    node_begin [_i] = _begin;                \
    node_end [_i] = _end;                    \
  } while (false)

void kdtree_t::compute (unsigned * new_index, const float (* new_x) [4], unsigned count)
{
  index = new_index;
  x = new_x;
  unsigned new_node_count = nodes_required (count);
  reallocate_aligned_arrays (memory, node_count, new_node_count,
                             & node_lohi, & node_begin, & node_end);

  __m128 klo = { -0x1.0p+126f, -0x1.0p+126f, -0x1.0p+126f, };
  __m128 khi = { +0x1.0p+126f, +0x1.0p+126f, +0x1.0p+126f, };
  SETNODE (0, klo, khi, 0, count);

  unsigned stack [50]; // Big enough.
  unsigned sp = 0;
  stack [sp ++] = 0;
  while (sp) {
    unsigned i = stack [-- sp];
    unsigned begin = node_begin [i];
    unsigned end = node_end [i];
    if (end - begin > 2) {
      unsigned middle = (begin + end) / 2;
      unsigned dim = node_dimension (i);
      partition (index, x, begin, middle, end, dim);
      __m128 lo = _mm_load_ps (node_lohi [i] [0]);
      __m128 hi = _mm_load_ps (node_lohi [i] [1]);
      float temp [2] [4];
      _mm_store_ps (temp [0], lo);
      _mm_store_ps (temp [1], hi);
      temp [0] [dim] = x [index [middle]] [dim];
      temp [1] [dim] = x [index [middle]] [dim];
      __m128 mid_lo = _mm_load_ps (temp [0]);
      __m128 mid_hi = _mm_load_ps (temp [1]);
      SETNODE (2 * i + 1, lo, mid_hi, begin, middle);
      SETNODE (2 * i + 2, mid_lo, hi, middle, end);
      stack [sp ++] = 2 * i + 2;
      stack [sp ++] = 2 * i + 1;
    }
  }
}

void kdtree_t::for_near (unsigned count, float r, void * data, callback_t f)
{
  __m128 rsq = _mm_set1_ps (r * r);
  for (unsigned k = 0; k != count; ++ k) {
    __m128 xx = _mm_load_ps (x [k]);
    __m128 zero = _mm_setzero_ps ();
    unsigned stack [50]; // Big enough.
    unsigned sp = 0;
    stack [sp ++] = 0;
    while (sp) {
      unsigned i = stack [-- sp];
      unsigned begin = node_begin [i];
      unsigned end = node_end [i];
      if (end - begin > 2) {
        // Does node i's box intersect the specified ball?
        __m128 lo = _mm_sub_ps (_mm_load_ps (node_lohi [i] [0]), xx);
        __m128 hi = _mm_sub_ps (xx, _mm_load_ps (node_lohi [i] [1]));
        __m128 mx = _mm_max_ps (lo, hi);
        __m128 nneg = _mm_and_ps (_mm_cmpge_ps (mx, zero), mx);
        __m128 dsq = dot (nneg, nneg);
        if (_mm_comilt_ss (dsq, rsq)) {
          float temp [4];
          _mm_store_ps (temp, dsq);
          unsigned dim = node_dimension (i);
          float mid = node_lohi [2 * i + 1] [1] [dim];
          if (x [k] [dim] >= mid - r) stack [sp ++] = 2 * i + 2;
          if (x [k] [dim] <= mid + r) stack [sp ++] = 2 * i + 1;
        }
      }
      else {
        for (unsigned n = node_begin [i]; n != node_end [i]; ++ n) {
          unsigned j = index [n];
          if (j > k)
            f (data, k, j);
        }
      }
    }
  }
}

void kdtree_t::for_near (float (* planes) [2] [4], float r, void * data, callback_t f)
{
  __m128 r0 = { r, 0.0f, 0.0f, 0.0f, };
  __m128 zero = _mm_setzero_ps ();
  for (unsigned k = 0; k != 6; ++ k) {
    __m128 a = _mm_load_ps (planes [k] [0]);
    __m128 n = _mm_load_ps (planes [k] [1]);
    __m128 mask = _mm_cmpge_ps (n, zero);
    unsigned stack [50]; // Big enough.
    unsigned sp = 0;
    stack [sp ++] = 0;
    while (sp) {
      unsigned i = stack [-- sp];
      unsigned begin = node_begin [i];
      unsigned end = node_end [i];
      if (end - begin > 2) {
        // Does node i's box intersect the specified half-plane?
        // Only need to test one corner.
        __m128 lo = _mm_load_ps (node_lohi [i] [0]);
        __m128 hi = _mm_load_ps (node_lohi [i] [1]);
        __m128 x = _mm_or_ps (_mm_and_ps (mask, lo), _mm_andnot_ps (mask, hi));
        __m128 d = dot (_mm_sub_ps (x, a), n);
        if (_mm_comilt_ss (d, r0)) {
          // There's no shortcut to reject boxes here as there was for the balls.
          stack [sp ++] = 2 * i + 2;
          stack [sp ++] = 2 * i + 1;
        }
      }
      else {
        for (unsigned n = node_begin [i]; n != node_end [i]; ++ n) {
          unsigned j = index [n];
          f (data, k, j);
        }
      }
    }
  }
}
