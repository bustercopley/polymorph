#include "mswin.h"
#include "kdtree.h"
#include "partition.h"
#include "memory.h"
#include "aligned-arrays.h"
#include "vector.h"
#include "compiler.h"
#include <limits>

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
    store4f (node_lohi [_i] [0], _lo);  \
    store4f (node_lohi [_i] [1], _hi);  \
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
  static const float inf = std::numeric_limits <float>::infinity ();
  __m128 klo = { -inf, -inf, -inf, 0.0f, };
  __m128 khi = { +inf, +inf, +inf, 0.0f, };
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
      v4f lo = load4f (node_lohi [i] [0]);
      v4f hi = load4f (node_lohi [i] [1]);
      float temp [2] [4] ALIGNED16;
      store4f (temp [0], lo);
      store4f (temp [1], hi);
      temp [0] [dim] = x [index [middle]] [dim];
      temp [1] [dim] = x [index [middle]] [dim];
      v4f mid_lo = load4f (temp [0]);
      v4f mid_hi = load4f (temp [1]);
      SETNODE (2 * i + 1, lo, mid_hi, begin, middle);
      SETNODE (2 * i + 2, mid_lo, hi, middle, end);
      stack [sp ++] = 2 * i + 2;
      stack [sp ++] = 2 * i + 1;
    }
  }
}

void kdtree_t::for_near (unsigned count, float r, void * data, callback_t f)
{
  v4f rsq = _mm_set1_ps (r * r);
  for (unsigned k = 0; k != count; ++ k) {
    v4f xx = load4f (x [k]);
    v4f zero = _mm_setzero_ps ();
    unsigned stack [50]; // Big enough.
    unsigned sp = 0;
    stack [sp ++] = 0;
    while (sp) {
      unsigned i = stack [-- sp];
      unsigned begin = node_begin [i];
      unsigned end = node_end [i];
      if (end - begin > 2) {
        // Does node i's box intersect the specified ball?
        v4f lo = load4f (node_lohi [i] [0]) - xx;
        v4f hi = xx - load4f (node_lohi [i] [1]);
        v4f mx = _mm_max_ps (lo, hi);
        v4f nneg = _mm_and_ps (_mm_cmpge_ps (mx, zero), mx);
        v4f dsq = dot (nneg, nneg);
        if (_mm_comilt_ss (dsq, rsq)) {
          float temp [4] ALIGNED16;
          store4f (temp, dsq);
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
  v4f r0 = _mm_set1_ps (r);
  v4f zero = _mm_setzero_ps ();
  for (unsigned k = 0; k != 6; ++ k) {
    v4f a = load4f (planes [k] [0]);
    v4f n = load4f (planes [k] [1]);
    v4f mask = _mm_cmpge_ps (n, zero);
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
        v4f lo = load4f (node_lohi [i] [0]);
        v4f hi = load4f (node_lohi [i] [1]);
        v4f x = _mm_or_ps (_mm_and_ps (mask, lo), _mm_andnot_ps (mask, hi));
        v4f d = dot (x - a, n);
        if (_mm_comilt_ss (d, r0)) {
          // There's no shortcut to reject boxes here as there was for the balls.
          stack [sp ++] = 2 * i + 2;
          stack [sp ++] = 2 * i + 1;
        }
      }
      else {
        for (unsigned n = begin; n != end; ++ n) {
          unsigned j = index [n];
          f (data, k, j);
        }
      }
    }
  }
}
