#include "bounce.h"
#include "model.h"
#include "vector.h"
#include "config.h"

void bounce (unsigned ix, unsigned iy, object_t * objects,
             const float (* x) [4], float (* v) [4], float (* w) [4])
{
  object_t & A = objects [ix];
  object_t & B = objects [iy];
  v4f s = { A.r + B.r, 0.0f, 0.0f, 0.0f, };
  v4f ssq = s * s;
  v4f dx = load4f (x [iy]) - load4f (x [ix]);
  v4f dxsq = dot (dx, dx);
  if (_mm_comilt_ss (dxsq, ssq)) { // spheres interpenetrate?
    v4f dv = load4f (v [iy]) - load4f (v [ix]);
    v4f dxdv = dot (dx, dv);
    v4f zero = _mm_setzero_ps ();
    if (_mm_comilt_ss (dxdv, zero)) { // spheres approach?
      v4f dxlen = _mm_sqrt_ps (dxsq);
      v4f dxn = dx / dxlen;
      v4f rw = _mm_set1_ps (A.r) * load4f (w [ix])
             + _mm_set1_ps (B.r) * load4f (w [iy]);
      v4f unn = dx * dxdv / dxsq;
      v4f rub = cross (rw, dxn) + unn - dv;
      v4f kf = _mm_set1_ps (usr::balls_friction);
      v4f u = kf * rub - unn;

      // Now take the nonzero multiplier lambda such that the
      // impulse lambda*u is consistent with conservation of
      // energy and momentum.

      v4f dxu = cross (dxn, u);
      v4f km2 = { -2.0f, -2.0f, -2.0f, -2.0f, };
      v4f top = km2 * (dot (u, dv) - dot (dxu, rw));
      v4f uu = dot (u, u);
      v4f r = { 1.0f, 1.0f, A.r, B.r, };
      v4f rdxu = (r * r) * dot (dxu, dxu);
      v4f urdxu = _mm_movehl_ps (rdxu, uu);
      v4f divisors = { A.m, B.m, A.l, B.l, };
      v4f quotients = urdxu / divisors;
      v4f ha = _mm_hadd_ps (quotients, quotients);
      v4f hh = _mm_hadd_ps (ha, ha);
      v4f munu = (top * r) / (hh * divisors);
      v4f muA, muB, nuA, nuB;
      UNPACK4 (munu, muA, muB, nuA, nuB);
      store4f (v [ix], load4f (v [ix]) - muA * u);
      store4f (v [iy], load4f (v [iy]) + muB * u);
      store4f (w [ix], load4f (w [ix]) - nuA * dxu);
      store4f (w [iy], load4f (w [iy]) - nuB * dxu);
    }
  }
}


void bounce (const float (& plane) [2] [4], unsigned ix, object_t * objects,
             const float (* x) [4], float (* v) [4], float (* w) [4])
{
  object_t & A = objects [ix];
  v4f anchor = load4f (plane [0]);
  v4f normal = load4f (plane [1]);
  v4f s = dot (load4f (x [ix]) - anchor, normal);
  v4f r = _mm_set1_ps (A.r);
  if (_mm_comilt_ss (s, r)) { // Sphere penetrates plane?
    v4f vn = dot (load4f (v [ix]), normal);
    v4f zero = _mm_setzero_ps ();
    if (_mm_comilt_ss (vn, zero)) { // Sphere approaches plane?
      // vN is the normal component of v. (The normal is a unit vector).
      // vF is the tangential contact velocity, composed of glide and spin.
      v4f vN = vn * normal;
      v4f rn = r * normal;
      v4f vF = load4f (v [ix]) - vN - cross (load4f (w [ix]), rn);
      v4f kf = _mm_set1_ps (usr::walls_friction);
      v4f uneg = vN + kf * vF;
      v4f vN_sq = vn * vn;
      v4f vF_sq = dot (vF, vF);
      v4f kvF_sq = kf * (kf * vF_sq);
      v4f ml = { A.m, A.l, A.m, A.l, };
      v4f mix = _mm_unpacklo_ps (vN_sq + kvF_sq, (r * r) * kvF_sq) / ml;
      v4f munu = (vN_sq + kf * vF_sq) / (ml * _mm_hadd_ps (mix, mix));
      munu += munu;
      munu = _mm_unpacklo_ps (munu, munu);
      v4f mu = _mm_movelh_ps (munu, munu);
      v4f nu = _mm_movehl_ps (munu, munu);
      store4f (v [ix], load4f (v [ix]) - mu * uneg);
      store4f (w [ix], load4f (w [ix]) + nu * cross (rn, uneg));
    }
  }
}

