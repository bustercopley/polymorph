// -*- C++ -*-
#ifndef vector_h
#define vector_h

#include <immintrin.h>
#include <cstdint>

// Interface.

typedef __m128 v4f;

#define SHUFFLE(a, b, c, d) ((a) | ((b) << 2) | ((c) << 4) | ((d) << 6))
#define UNPACK2(a,a0,a1)               \
do {                                   \
  v4f _t = (a);                        \
  v4f _t0 = _mm_unpacklo_ps (_t, _t);  \
  (a0) = _mm_movelh_ps (_t0, _t0);     \
  (a1) = _mm_movehl_ps (_t0, _t0);     \
} while (0)
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
#define TRANSPOSE3(a,b,c,a0,a1,a2)    \
do {                                   \
  v4f _t = (a);                        \
  v4f _u = (b);                        \
  v4f _v = (c);                        \
  v4f _w = _mm_setzero_ps ();          \
  v4f _t0 = _mm_unpacklo_ps (_t, _u);  \
  v4f _t1 = _mm_unpacklo_ps (_v, _w);  \
  v4f _t2 = _mm_unpackhi_ps (_t, _u);  \
  v4f _t3 = _mm_unpackhi_ps (_v, _w);  \
  (a0) = _mm_movelh_ps (_t0, _t2);     \
  (a1) = _mm_movehl_ps (_t2, _t0);     \
  (a2) = _mm_movelh_ps (_t1, _t3);     \
} while (0)

inline v4f load4f (const float * a) { return _mm_load_ps (a); }
inline void store4f (float * a, v4f v) { _mm_store_ps (a, v); }
inline void move4f (float * dst, float * src) { store4f (dst, load4f (src)); }
inline v4f broadcast0 (v4f x) { return _mm_shuffle_ps (x, x, SHUFFLE (0, 0, 0, 0)); }
inline v4f broadcast1 (v4f x) { return _mm_shuffle_ps (x, x, SHUFFLE (1, 1, 1, 1)); }
inline v4f broadcast2 (v4f x) { return _mm_shuffle_ps (x, x, SHUFFLE (2, 2, 2, 2)); }
inline v4f broadcast3 (v4f x) { return _mm_shuffle_ps (x, x, SHUFFLE (3, 3, 3, 3)); }

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

inline v4f cross (const v4f a, const v4f b)
{
  v4f c = _mm_shuffle_ps (a, a, SHUFFLE (1, 2, 0, 3)); // a1 a2 a0 a3
  v4f d = _mm_shuffle_ps (b, b, SHUFFLE (1, 2, 0, 3)); // b1 b2 b0 b3
  v4f r1 = a * d - b * c;          // a0b1-b0a1 a1b2-b1a2 a2b0-b2a0 0
  v4f r = _mm_shuffle_ps (r1, r1, SHUFFLE (1, 2, 0, 3));
  return r;
}

// Reciprocal, x = 1/k.
// Newton's method on f(x) = (1/x) - k.
inline v4f rcp (v4f k)
{
  v4f x0 = _mm_rcp_ps (k);
  return (x0 + x0) - k * (x0 * x0);
}

// Reciprocal square root, x = 1/sqrt(k).
// Newton's method on f(x) = (1/(x^2)) - k.
inline v4f rsqrt (v4f k)
{
  v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
  v4f three = { 3.0f, 3.0f, 3.0f, 3.0f, };
  v4f x0 = _mm_rsqrt_ps (k);
  return (half * x0) * (three - (x0 * x0) * k);
}

inline v4f normalize (v4f v)
{
  return v * rsqrt (dot (v, v));
}

inline v4f sqrt (v4f k)
{
  v4f zero = { 0.0f, 0.0f, 0.0f, 0.0f, };
  return k * _mm_and_ps (_mm_cmpneq_ps (k, zero), rsqrt (k));
}

// // Apply the 3x3 matrix m to the column vector x.
// inline v4f mapply (const float (& m) [3] [4], v4f x)
// {
//   v4f r0 = dot (load4f (m [0]), x);   // r0 r0 r0 r0
//   v4f r1 = dot (load4f (m [1]), x);   // r1 r1 r1 r1
//   v4f r2 = dot (load4f (m [2]), x);   // r2 r2 r2 r2
//   v4f r3 = _mm_setzero_ps ();         // r3 r3 r3 r3
//   v4f t0 = _mm_unpacklo_ps (r0, r1);  // r0 r1 r0 r1
//   v4f t1 = _mm_unpacklo_ps (r2, r3);  // r2  0 r2  0
//   return _mm_movelh_ps (t0, t1);      // r0 r1 r2 0
// }

// Transpose 3x3 matrix m and apply to column vector x.
inline v4f tmapply (const float (& m) [3] [4], v4f x)
{
  v4f m0 = load4f (m [0]);
  v4f m1 = load4f (m [1]);
  v4f m2 = load4f (m [2]);
  v4f x0, x1, x2;
  UNPACK3 (x, x0, x1, x2);
  return x0 * m0 + x1 * m1 + x2 * m2;
}

#endif
