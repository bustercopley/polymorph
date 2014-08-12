#include "model.h"
#include "vector.h"

namespace usr
{
  static const float walls_friction = 0.075f;
  static const float balls_friction = 0.075f;
}

// Called when two balls might collide.
// Maybe apply equal and opposite impulses to objects ix and iy.
inline void bounce (unsigned ix, unsigned iy,
                    object_t * RESTRICT objects,
                    const float (* RESTRICT r), const float (* RESTRICT x) [4],
                    float (* RESTRICT v) [4], float (* RESTRICT w) [4])
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
      // energy and momentum. See "doc/elastic.tex" for a sane
      // version of the disturbing ad-hoc calculation below.
      v4f dxu = cross (dxn, u);
      v4f km2 = { -2.0f, -2.0f, -2.0f, -2.0f, };
      v4f top = km2 * (dot (u, dv) - dot (dxu, rw));
      v4f usq = dot (u, u);
      v4f dxusq = dot (dxu, dxu);
      v4f R = { 1.0f, 1.0f, r [ix], r [iy], };
      v4f urdxu_sq = _mm_movehl_ps (R * R * dxusq, usq);
      v4f divisors = { A.m, B.m, A.l, B.l, };
      v4f quotients = urdxu_sq / divisors;
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

// Called when a ball might collide with a wall.
inline void bounce (unsigned iw, unsigned ix,
                    const float (* RESTRICT walls) [2] [4],
                    object_t * RESTRICT objects,
                    const float (* RESTRICT r), const float (* RESTRICT x) [4],
                    float (* RESTRICT v) [4], float (* RESTRICT w) [4])
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
      // A comprehensible version is left as an exercise to the reader.
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