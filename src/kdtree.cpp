#include "kdtree.h"
#include "partition.h"
#include "memory.h"
#include "aligned-arrays.h"
#include "model.h"
#include "vector.h"
#include "compiler.h"
#include "bsr.h"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace usr
{
  static const float walls_friction = 0.075f;
  static const float balls_friction = 0.075f;
}

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

  inline void bounce (object_t * RESTRICT objects,
                      const float (* RESTRICT r), const float (* RESTRICT x) [4],
                      float (* RESTRICT v) [4], float (* RESTRICT w) [4],
                      unsigned ix, unsigned iy)
  {
    const object_t & A = objects [ix];
    const object_t & B = objects [iy];
    v4f s = { r [ix] + r [iy], 0.0f, 0.0f, 0.0f, };
    v4f ssq = s * s;
    v4f dx = load4f (x [iy]) - load4f (x [ix]);
    v4f dxsq = dot (dx, dx);
    if (_mm_comilt_ss (dxsq, ssq)) { // Spheres interpenetrate?
      v4f dv = load4f (v [iy]) - load4f (v [ix]);
      v4f dxdv = dot (dx, dv);
      v4f zero = _mm_setzero_ps ();
      if (_mm_comilt_ss (dxdv, zero)) { // Spheres approach?
        v4f dxn = normalize (dx);
        v4f rw = _mm_set1_ps (r [ix]) * load4f (w [ix])
               + _mm_set1_ps (r [iy]) * load4f (w [iy]);
        v4f unn = dx * dxdv / dxsq;
        v4f rub = cross (rw, dxn) + unn - dv;
        v4f kf = _mm_set1_ps (usr::balls_friction);
        v4f u = kf * rub - unn;

        // Now take the nonzero multiplier lambda such that the
        // impulse lambda*u is consistent with conservation of
        // energy and momentum.
        // (Lambda is implicit now. See "elastic.tex".)

        v4f dxu = cross (dxn, u);
        v4f km2 = { -2.0f, -2.0f, -2.0f, -2.0f, };
        v4f top = km2 * (dot (u, dv) - dot (dxu, rw));
        v4f uu = dot (u, u);
        v4f R = { 1.0f, 1.0f, r [ix], r [iy], };
        v4f rdxu = (R * R) * dot (dxu, dxu);
        v4f urdxu = _mm_movehl_ps (rdxu, uu);
        v4f divisors = { A.m, B.m, A.l, B.l, };
        v4f quotients = urdxu / divisors;
        v4f ha = _mm_hadd_ps (quotients, quotients);
        v4f hh = _mm_hadd_ps (ha, ha);
        v4f munu = (top * R) / (hh * divisors);
        v4f muA, muB, nuA, nuB;
        UNPACK4 (munu, muA, muB, nuA, nuB);
        store4f (v [ix], load4f (v [ix]) - muA * u);
        store4f (v [iy], load4f (v [iy]) + muB * u);
        store4f (w [ix], load4f (w [ix]) - nuA * dxu);
        store4f (w [iy], load4f (w [iy]) - nuB * dxu); // sic
      }
    }
  }

  inline void bounce (object_t * RESTRICT objects,
                      const float (* RESTRICT r), const float (* RESTRICT x) [4],
                      float (* RESTRICT v) [4], float (* RESTRICT w) [4],
                      float (* RESTRICT walls) [2] [4],
                      unsigned iw, unsigned ix)
  {
    object_t & A = objects [ix];
    v4f anchor = load4f (walls [iw] [0]);
    v4f normal = load4f (walls [iw] [1]);
    v4f s = dot (load4f (x [ix]) - anchor, normal);
    v4f R = _mm_set1_ps (r [ix]);
    if (_mm_comilt_ss (s, R)) { // Sphere penetrates plane?
      v4f vn = dot (load4f (v [ix]), normal);
      v4f zero = _mm_setzero_ps ();
      if (_mm_comilt_ss (vn, zero)) { // Sphere approaches plane?
        // vN is the normal component of v. (The normal is a unit vector).
        // vF is the tangential contact velocity, composed of glide and spin.
        v4f vN = vn * normal;
        v4f rn = R * normal;
        v4f vF = load4f (v [ix]) - vN - cross (load4f (w [ix]), rn);
        v4f kf = _mm_set1_ps (usr::walls_friction);
        v4f kvF = kf * vF;
        v4f uneg = vN + kvF;
        v4f vN_sq = vn * vn;
        v4f k_vFsq = dot (kvF, vF);
        v4f kvF_sq = kf * k_vFsq;
        v4f ml = { A.m, A.l, A.m, A.l, };
        v4f wtf = _mm_unpacklo_ps (vN_sq + kvF_sq, (R * R) * kvF_sq) / ml;
        v4f munu = (vN_sq + k_vFsq) / (ml * _mm_hadd_ps (wtf, wtf));
        munu += munu;
        munu = _mm_unpacklo_ps (munu, munu);
        v4f mu = _mm_movelh_ps (munu, munu);
        v4f nu = _mm_movehl_ps (munu, munu);
        store4f (v [ix], load4f (v [ix]) - mu * uneg);
        store4f (w [ix], load4f (w [ix]) + nu * cross (rn, uneg));
      }
    }
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
