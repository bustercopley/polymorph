#include "bounce.h"
#include "object.h"
#include "vector.h"
#include "config.h"
#include "maths.h"

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
      v4f lambda = top / hh;

      float mu [4];
      store4f (mu, lambda * r / divisors);
      store4f (v [ix], load4f (v [ix]) - _mm_set1_ps (mu [0]) * u);
      store4f (v [iy], load4f (v [iy]) + _mm_set1_ps (mu [1]) * u);
      store4f (w [ix], load4f (w [ix]) - _mm_set1_ps (mu [2]) * dxu);
      store4f (w [iy], load4f (w [iy]) - _mm_set1_ps (mu [3]) * dxu);
    }
  }
}

void bounce (const float (& plane) [2] [4], unsigned ix, object_t * objects,
             const float (* x) [4], float (* v) [4], float (* w) [4])
{
  v4f anchor = load4f (plane [0]);
  v4f normal = load4f (plane [1]);
  v4f zero = _mm_setzero_ps ();
  object_t & A = objects [ix];
  v4f r = _mm_set1_ps (A.r);
  v4f s = dot (load4f (x [ix]) - anchor, normal);
  if (_mm_comilt_ss (s, r)) { // Sphere penetrates plane?
    v4f vv = load4f (v [ix]);
    v4f vn = dot (vv, normal);
    if (_mm_comilt_ss (vn, zero)) { // Sphere approaches plane?
      v4f ww = load4f (w [ix]);
      // vN is the normal component of v. (The normal is a unit vector).
      // vF is the tangential contact velocity, composed of glide and spin.
      v4f vN = vn * normal;
      v4f vF = vv - vN - r * cross (ww, normal);
      float kf = usr::walls_friction;
      v4f uneg = vN + _mm_set1_ps (kf) * vF;
      float vN_sq = _mm_cvtss_f32 (vn * vn);
      float vF_sq = _mm_cvtss_f32 (dot (vF, vF));
      float kvF_sq = kf * kf * vF_sq;
      float lambda = 2 * (vN_sq + kf * vF_sq) / ((vN_sq + kvF_sq) / A.m + (A.r * A.r) * kvF_sq / A.l);
      store4f (v [ix], load4f (v [ix]) - _mm_set1_ps (lambda / A.m) * uneg);
      store4f (w [ix], load4f (w [ix]) + _mm_set1_ps (lambda * A.r / A.l) * cross (normal, uneg));
    }
  }
}
