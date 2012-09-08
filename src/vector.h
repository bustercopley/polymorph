// -*- C++ -*-

#ifndef vector_h
#define vector_h

#include <immintrin.h>
#include <cstdint>

// Interface.

typedef __m128 v4f;
typedef __m128d v2d;

#define VECTOR for (unsigned i = 0; i != 3; ++ i)
#define VECTOR_ijk for (unsigned i = 0, j = 1, k = 2; i != 3; j = k, k = i, ++ i)
#define CROSS(a, b) ((a) [j] * (b) [k] - (a) [k] * (b) [j])
#define SHUFFLE(a, b, c, d) ((a) | ((b) << 2) | ((c) << 4) | ((d) << 6))
#define UNPACK3(a,a0,a1,a2)            \
do {                                   \
  v4f _t = (a);                        \
  v4f _t0 = _mm_unpacklo_ps (_t, _t);  \
  v4f _t1 = _mm_unpackhi_ps (_t, _t);  \
  (a0) = _mm_movelh_ps (_t0, _t0);     \
  (a1) = _mm_movehl_ps (_t0, _t0);     \
  (a2) = _mm_movelh_ps (_t1, _t1);     \
} while (0)
#define UNPACK4(a,a0,a1,a2,a3)         \
do {                                   \
  v4f _t = (a);                        \
  v4f _t0 = _mm_unpacklo_ps (_t, _t);  \
  v4f _t1 = _mm_unpackhi_ps (_t, _t);  \
  (a0) = _mm_movelh_ps (_t0, _t0);     \
  (a1) = _mm_movehl_ps (_t0, _t0);     \
  (a2) = _mm_movelh_ps (_t1, _t1);     \
  (a3) = _mm_movehl_ps (_t1, _t1);     \
} while (0)

inline v4f load4f (const float * a) { return _mm_load_ps (a); }
inline v2d load2d (const double * a) { return _mm_load_pd (a); }
inline void store4f (float * a, v4f v) { _mm_store_ps (a, v); }
inline void store2d (double * a, v2d v) { _mm_store_pd (a, v); }
inline v4f dot (v4f u, v4f v);
inline v2d dot (const double (& u) [4], const double (& v) [4]);
inline v4f cross (v4f u, v4f v);
inline v4f mmul (v4f m0, v4f m1, v4f m2, v4f v);
inline v4f broadcast0 (v4f x);
inline v4f broadcast1 (v4f x);
inline v4f broadcast2 (v4f x);
inline v4f broadcast3 (v4f x);

// Implementation.

inline v4f broadcast0 (v4f x) { v4f t = _mm_unpacklo_ps (x, x); return _mm_movelh_ps (t, t); }
inline v4f broadcast1 (v4f x) { v4f t = _mm_unpacklo_ps (x, x); return _mm_movehl_ps (t, t); }
inline v4f broadcast2 (v4f x) { v4f t = _mm_unpackhi_ps (x, x); return _mm_movelh_ps (t, t); }
inline v4f broadcast3 (v4f x) { v4f t = _mm_unpackhi_ps (x, x); return _mm_movehl_ps (t, t); }

inline v4f dot (v4f u, v4f v)
{
#ifdef __SSE4_1__
  return _mm_dp_ps (u, v, 0xff);
#else
  v4f uv = u * v;
  v4f ha = _mm_hadd_ps (uv, uv);
  return _mm_hadd_ps (ha, ha);
#endif
}

inline v2d dot (const double (& u) [4], const double (& v) [4])
{
  v2d u0 = load2d (& u [0]);
  v2d u1 = load2d (& u [2]);
  v2d v0 = load2d (& v [0]);
  v2d v1 = load2d (& v [2]);
  v2d ha = _mm_hadd_pd (u0 * v0, u1 * v1); // u0v0+u1v1 u2v2+u3v3
  return _mm_hadd_pd (ha, ha);
}

inline v4f cross (const v4f a, const v4f b)
{
  v4f c = _mm_shuffle_ps (a, a, SHUFFLE (1, 2, 0, 3)); // a1 a2 a0 a3
  v4f d = _mm_shuffle_ps (b, b, SHUFFLE (1, 2, 0, 3)); // b1 b2 b0 b3
  v4f r1 = a * d - b * c;          // a0b1-b0a1 a1b2-b1a2 a2b0-b2a0 0
  v4f r = _mm_shuffle_ps (r1, r1, SHUFFLE (1, 2, 0, 3));
  return r;
}

#endif
