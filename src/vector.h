// -*- C++ -*-

#ifndef vector_h
#define vector_h

#include <immintrin.h>

// Interface.

#define VECTOR for (unsigned i = 0; i != 3; ++ i)
#define VECTOR_ijk for (unsigned i = 0, j = 1, k = 2; i != 3; j = k, k = i, ++ i)
#define CROSS(a, b) ((a) [j] * (b) [k] - (a) [k] * (b) [j])
#define SHUFFLE(a, b, c, d) ((a) | ((b) << 2) | ((c) << 4) | ((d) << 6))

inline __m128 dot (__m128 u, __m128 v);
inline __m128 cross (__m128 u, __m128 v);
inline __m128d dot (const double (& u) [4], const double (& v) [4]);
inline float dot (const float (& u) [4], const float (& v) [4]);

// Implementation.

inline __m128 dot (__m128 u, __m128 v)
{
#ifdef __SSE4_1__
  return _mm_dp_ps (u, v, 0xff);
#else
  __m128 uv = _mm_mul_ps (u, v);
  __m128 ha = _mm_hadd_ps (uv, uv);
  return _mm_hadd_ps (ha, ha);
#endif
}

inline __m128 cross (const __m128 x, const __m128 y)
{
  // __m128 a = _mm_shuffle_ps (x, x, SHUFFLE (1, 2, 0, 3)); // x1 x2 x0 x3
  // __m128 b = _mm_shuffle_ps (x, x, SHUFFLE (2, 0, 1, 3)); // x2 x0 x1 x3
  // __m128 c = _mm_shuffle_ps (y, y, SHUFFLE (1, 2, 0, 3)); // y1 y2 y0 y3
  // __m128 d = _mm_shuffle_ps (y, y, SHUFFLE (2, 0, 1, 3)); // y2 y0 y1 y3
  // __m128 ad = _mm_mul_ps (a, d);
  // __m128 bc = _mm_mul_ps (b, c);
  // __m128 r = _mm_sub_ps (ad, bc);
  // return r;
  __m128 z = _mm_shuffle_ps (x, x, SHUFFLE (1, 2, 0, 3)); // x1 x2 x0 x3
  __m128 w = _mm_shuffle_ps (y, y, SHUFFLE (1, 2, 0, 3)); // y1 y2 y0 y3
  __m128 xw = _mm_mul_ps (x, w);  // x0y1 x1y2 x2y0 x3y3
  __m128 zy = _mm_mul_ps (z, y);  // x1y0 x2y1 x0y2 x3y3
  __m128 r1 = _mm_sub_ps (xw, zy);
  __m128 r = _mm_shuffle_ps (r1, r1, SHUFFLE (1, 2, 0, 3));
  return r;
}

inline float dot (const float (& u) [4], const float (& v) [4])
{
  float result [4];
  _mm_store_ps (result, dot (_mm_load_ps (u), _mm_load_ps (v)));
  return result [0];
}

inline __m128d dot (const double (& u) [4], const double (& v) [4])
{
  __m128d u0 = _mm_load_pd (& u [0]);
  __m128d u1 = _mm_load_pd (& u [2]);
  __m128d v0 = _mm_load_pd (& v [0]);
  __m128d v1 = _mm_load_pd (& v [2]);
  __m128d t0 = _mm_mul_pd (u0, v0);  // uv0 uv1
  __m128d t1 = _mm_mul_pd (u1, v1);  // uv2 uv3
  __m128d ha = _mm_hadd_pd (t0, t1); // uv0+uv1 uv2+uv3
  return _mm_hadd_pd (ha, ha);
}

// Store the sum of the three 3-vectors u, v and w into p.
template <typename T>
inline void add (const T (& u) [3], const T (& v) [3], const T (& w) [3], T (& p) [3]) {
  VECTOR p [i] = u [i] + v [i] + w [i];
}

// Store the scalar product of s by the 3-vector u into v.
template <typename T>
inline void scalar_multiply (T s, const T (& u) [3], T (& v) [3]) {
  VECTOR v [i] = s * u [i];
}

// Return the scalar product of the 3-vectors u and v.
template <typename T>
inline T dot (const T (& u) [3], const T (& v) [3]) {
  T result = 0;
  VECTOR result += u [i] * v [i];
  return result;
}

#endif
