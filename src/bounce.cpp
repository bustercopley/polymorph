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
  __m128 s = { A.r + B.r, 0.0f, 0.0f, 0.0f, };
  __m128 ssq = _mm_mul_ps (s, s);
  __m128 dx = _mm_sub_ps (_mm_load_ps (x [iy]), _mm_load_ps (x [ix]));
  __m128 dxsq = dot (dx, dx);
  if (_mm_comilt_ss (dxsq, ssq)) { // spheres interpenetrate?
    __m128 dv = _mm_sub_ps (_mm_load_ps (v [iy]), _mm_load_ps (v [ix]));
    __m128 dxdv = dot (dx, dv);
    __m128 zero = _mm_setzero_ps ();
    if (_mm_comilt_ss (dxdv, zero)) { // spheres approach?
      __m128 dxlen = _mm_sqrt_ps (dxsq);
      __m128 dxn = _mm_div_ps (dx, dxlen);
      __m128 rw = _mm_add_ps (_mm_mul_ps (_mm_set1_ps (A.r), _mm_load_ps (w [ix])),
                              _mm_mul_ps (_mm_set1_ps (B.r), _mm_load_ps (w [iy])));
      __m128 unn = _mm_mul_ps (_mm_div_ps (dxdv, dxsq), dx);
      __m128 rub = _mm_add_ps (cross (rw, dxn), _mm_sub_ps (unn, dv));
      __m128 kf = _mm_set1_ps (usr::balls_friction);
      __m128 u = _mm_sub_ps (_mm_mul_ps (kf, rub), unn);

      // Now take the nonzero multiplier lambda such that the
      // impulse lambda*u is consistent with conservation of
      // energy and momentum.

      __m128 dxu = cross (dxn, u);
      __m128 km2 = { -2.0f, -2.0f, -2.0f, -2.0f, };
      __m128 top = _mm_mul_ps (km2, _mm_sub_ps (dot (u, dv), dot (dxu, rw)));
      __m128 uu = dot (u, u);
      __m128 r = { 1.0f, 1.0f, A.r, B.r, };
      __m128 rdxu = _mm_mul_ps (_mm_mul_ps (r, r), dot (dxu, dxu));
      __m128 urdxu = _mm_movehl_ps (rdxu, uu);
      __m128 divisors = { A.m, B.m, A.l, B.l, };
      __m128 quotients = _mm_div_ps (urdxu, divisors);
      __m128 ha = _mm_hadd_ps (quotients, quotients);
      __m128 hh = _mm_hadd_ps (ha, ha);
      __m128 lambda = _mm_div_ps (top, hh);

      float mu [4];
      _mm_store_ps (mu, _mm_div_ps (_mm_mul_ps (lambda, r), divisors));
      _mm_store_ps (v [ix], _mm_sub_ps (_mm_load_ps (v [ix]), _mm_mul_ps (_mm_set1_ps (mu [0]), u)));
      _mm_store_ps (v [iy], _mm_add_ps (_mm_load_ps (v [iy]), _mm_mul_ps (_mm_set1_ps (mu [1]), u)));
      _mm_store_ps (w [ix], _mm_sub_ps (_mm_load_ps (w [ix]), _mm_mul_ps (_mm_set1_ps (mu [2]), dxu)));
      _mm_store_ps (w [iy], _mm_sub_ps (_mm_load_ps (w [iy]), _mm_mul_ps (_mm_set1_ps (mu [3]), dxu)));
    }
  }
}

void bounce (const float (& plane) [2] [4], unsigned ix, object_t * objects,
             const float (* x) [4], float (* v) [4], float (* w) [4])
{
  __m128 anchor = _mm_load_ps (plane [0]);
  __m128 normal = _mm_load_ps (plane [1]);
  __m128 zero = _mm_setzero_ps ();
  object_t & A = objects [ix];
  __m128 r = _mm_set1_ps (A.r);
  __m128 s = dot (_mm_sub_ps (_mm_load_ps (x [ix]), anchor), normal);
  if (_mm_comilt_ss (s, r)) { // Sphere penetrates plane?
    __m128 vv = _mm_load_ps (v [ix]);
    __m128 vn = dot (vv, normal);
    if (_mm_comilt_ss (vn, zero)) { // Sphere approaches plane?
      __m128 ww = _mm_load_ps (w [ix]);
      __m128 vN = _mm_mul_ps (vn, normal);
      __m128 vF = _mm_sub_ps (vv, _mm_add_ps (vN, _mm_mul_ps (r, cross (ww, normal))));
      // vN is the normal component of v. (The normal is a unit vector).
      // vF is the tangential contact velocity, composed of glide and spin.
      float kf = usr::walls_friction;
      __m128 uneg = _mm_add_ps (vN, _mm_mul_ps (_mm_set1_ps (kf), vF));
      float vN_sq = _mm_cvtss_f32 (vn * vn);
      float vF_sq = _mm_cvtss_f32 (dot (vF, vF));
      float kvF_sq = kf * kf * vF_sq;
      float lambda = 2 * (vN_sq + kf * vF_sq) / ((vN_sq + kvF_sq) / A.m + (A.r * A.r) * kvF_sq / A.l);
      _mm_store_ps (v [ix], _mm_sub_ps (_mm_load_ps (v [ix]), _mm_mul_ps (_mm_set1_ps (lambda / A.m), uneg)));
      _mm_store_ps (w [ix], _mm_add_ps (_mm_load_ps (w [ix]), _mm_mul_ps (_mm_set1_ps (lambda * A.r / A.l), cross (normal, uneg))));
    }
  }
}
