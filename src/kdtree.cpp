#include "kdtree.h"
#include "partition.h"
#include "bounce.h"
#include "model.h"
#include "aligned-arrays.h"
#include "memory.h"
#include "compiler.h"
#include "vector.h"
#include "bsr.h"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace
{
  inline unsigned nodes_required (unsigned n)
  {
    return (2u << bsr (n - 1u)) - 1u;
  }

  inline unsigned node_dimension (unsigned i)
  {
    return bsr (i + 1u) % 3u;
  }

  inline v4f dim_mask (unsigned dim)
  {
    ALIGNED16 static const union {
      std::uint32_t u32 [4];
      float f32 [4];
    } masks [3] = {
      { { 0xffffffff, 0, 0, 0, }, },
      { { 0, 0xffffffff, 0, 0, }, },
      { { 0, 0, 0xffffffff, 0, }, },
    };
    return load4f (masks [dim].f32);
  }
}

kdtree_t::~kdtree_t ()
{
  deallocate (memory);
}

#define SETNODE(_i, _lo, _hi, _begin, _end)  \
  do {                                       \
    store4f (node_lohi [_i] [0], _lo);       \
    store4f (node_lohi [_i] [1], _hi);       \
    node_begin [_i] = _begin;                \
    node_end [_i] = _end;                    \
  } while (false)

bool kdtree_t::compute (unsigned * RESTRICT new_index, const float (* RESTRICT new_x) [4], unsigned count)
{
  index = new_index;
  x = new_x;
  // Undefined behaviour if count == 0.
  unsigned new_node_count = nodes_required (count);
  if (! reallocate_aligned_arrays (memory, node_count, new_node_count,
                                   & node_lohi, & node_begin, & node_end))
    return false;
  static const float inf = std::numeric_limits <float>::infinity ();
  v4f klo = { -inf, -inf, -inf, 0.0f, };
  v4f khi = { +inf, +inf, +inf, 0.0f, };
  SETNODE (0, klo, khi, 0, count);
  unsigned stack [50]; // Big enough.
  unsigned sp = 0;
  stack [sp ++] = 0;
  while (sp) {
    unsigned i = stack [-- sp];
    unsigned begin = node_begin [i];
    unsigned end = node_end [i];
    if (end - begin > 2) {
      unsigned dim = node_dimension (i);
      unsigned middle = begin + (end - begin) / 2;
      partition (index, x, dim, begin, middle, end);
      v4f lo = load4f (node_lohi [i] [0]);
      v4f hi = load4f (node_lohi [i] [1]);
      v4f mid = load4f (x [index [middle]]);
      v4f dm = dim_mask (dim);
      v4f md = _mm_and_ps (dm, mid);
      v4f mid_lo = _mm_or_ps (md, _mm_andnot_ps (dm, lo));
      v4f mid_hi = _mm_or_ps (md, _mm_andnot_ps (dm, hi));
      SETNODE (2 * i + 1, lo, mid_hi, begin, middle);
      SETNODE (2 * i + 2, mid_lo, hi, middle, end);
      stack [sp ++] = 2 * i + 2;
      stack [sp ++] = 2 * i + 1;
    }
  }
  return true;
}

void kdtree_t::search (unsigned count, float R,
                       object_t * RESTRICT objects,
                       const float (* RESTRICT r),
                       float (* RESTRICT v) [4], float (* RESTRICT w) [4],
                       float (* RESTRICT walls) [2] [4]) const
{
  // For each ball, find nearby balls and test for collisions.
  v4f rsq = _mm_set1_ps (R * R);
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
          stack [sp ++] = 2 * i + 2;
          stack [sp ++] = 2 * i + 1;
        }
      }
      else {
        for (unsigned n = node_begin [i]; n != node_end [i]; ++ n) {
          unsigned j = index [n];
          if (j > k) {
            bounce (objects, r, x, v, w, j, k);
          }
        }
      }
    }
  }

  // For each wall, find nearby balls and test for collisions.
  v4f r0 = _mm_set1_ps (R);
  v4f zero = _mm_setzero_ps ();
  for (unsigned k = 0; k != 6; ++ k) {
    v4f a = load4f (walls [k] [0]);
    v4f n = load4f (walls [k] [1]);
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
          stack [sp ++] = 2 * i + 2;
          stack [sp ++] = 2 * i + 1;
        }
      }
      else {
        for (unsigned n = begin; n != end; ++ n) {
          unsigned j = index [n];
          bounce (objects, r, x, v, w, walls, k, j);
        }
      }
    }
  }
}
