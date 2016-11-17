// -*- C++ -*-

#ifndef vector_h
#define vector_h

#include "compiler.h"
#include "mswin.h"
#include <cstdint>
#include <immintrin.h>
typedef __m128 v4f;

#define SHUFFLE(a, b, c, d) ((a) | ((b) << 2) | ((c) << 4) | ((d) << 6))

ALWAYS_INLINE inline v4f load4f (const float * a) { return _mm_load_ps (a); }
ALWAYS_INLINE inline void store4f (float * a, v4f v) { _mm_store_ps (a, v); }

inline v4f dot (v4f u, v4f v)
{
#ifdef __SSE4_1__
  return _mm_dp_ps (u, v, 0x7f);
#else
  v4f uv = u * v;
  v4f ha = _mm_hadd_ps (uv, uv);
  return _mm_hadd_ps (ha, ha);
#endif
}

ALWAYS_INLINE inline v4f cross (const v4f a, const v4f b)
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

ALWAYS_INLINE inline v4f normalize (v4f v)
{
  return v * rsqrt (dot (v, v));
}

ALWAYS_INLINE inline v4f sqrt_nonzero (v4f k)
{
  return k * rsqrt (k);
}

ALWAYS_INLINE inline v4f sqrt (v4f k)
{
  return _mm_and_ps (_mm_cmpneq_ps (k, _mm_setzero_ps ()), k * rsqrt (k));
}

// Multiply column-major 3x3 matrix m (on the left) and column vector x (on the right).
ALWAYS_INLINE inline v4f mapply (const float (* m) [4], v4f x)
{
  v4f m0 = load4f (m [0]);
  v4f m1 = load4f (m [1]);
  v4f m2 = load4f (m [2]);

  v4f x0 = _mm_shuffle_ps (x, x, SHUFFLE (0, 0, 0, 0));
  v4f x1 = _mm_shuffle_ps (x, x, SHUFFLE (1, 1, 1, 1));
  v4f x2 = _mm_shuffle_ps (x, x, SHUFFLE (2, 2, 2, 2));

  return x0 * m0 + x1 * m1 + x2 * m2;
}

// Evaluate two cubic polynomials at t by Estrin's method. Requires SSE3 (for haddps).
// First argument t t * *, result a(t) b(t) a(t) b(t).
ALWAYS_INLINE inline v4f polyeval (const v4f t, const v4f a, const v4f b)
{
  v4f one = _mm_set1_ps (1.0f);        // 1 1 1 1
  v4f t1 = _mm_unpacklo_ps (one, t);   // 1 t 1 t
  v4f t1sq = t1 * t1;                  // squares of t1
  v4f c = a * t1;                      // a0 a1t a2 a3t
  v4f d = b * t1;                      // b0 b1t b2 b3t
  v4f e = _mm_hadd_ps (c, d);          // a0+a1t a2+a3t b0+b1t b2+b3t
  v4f f = e * t1sq;                    // a0+a1t a2t^2+a3t^3 b0+b1t b2t^2+b3t^3
  v4f g = _mm_hadd_ps (f, f);          // a(t) b(t) a(t) b(t)
  return g;
}

// Evaluate a single degree-7 polynomial at t by Estrin's method. Requires SSE3 (for haddps).
ALWAYS_INLINE inline float polyeval7 (const float t, const v4f lo, const v4f hi)
{
  v4f one = _mm_set1_ps (1.0f);       // 1 1 1 1
  v4f t0 = _mm_set1_ps (t);           // t t t t
  v4f t1 = _mm_unpacklo_ps (one, t0); // 1 t 1 t
  v4f t1sq = t1 * t1;                 // squares of t1
  v4f t1bq = t1sq * t1sq;             // biquadrates of t1
  v4f c = lo * t1;                    // l0 l1t l2 l3t
  v4f d = hi * t1;                    // h0 h1t h2 h3t
  v4f e = _mm_hadd_ps (c, d);         // l0+l1t l2+l3t h0+h1t h2+h3t
  v4f f = e * t1sq;                   // l0+l1t l2t^2+l3t^3 h0+h1t h2t^2+h3t^3
  v4f g = _mm_hadd_ps (f, f);         // l0+l1t+l2t^2+l3t^3 h0+h1t+h2t^2+h3t^3 * *
  v4f h = g * t1bq;                   // l0+l1t+l2t^2+l3t^3 h0t^4+h1t^5+h2t^6+h3t^7 * *
  v4f i = _mm_hadd_ps (h, h);         // l0+l1t+l2t^2+l3t^3+h0t^4+h1t^5+h2t^6+h3t^7 * * *
  return _mm_cvtss_f32 (i);
}

// Scalar utility functions

// Convert non-negative float to unsigned int, rounding down.
ALWAYS_INLINE inline std::uint32_t truncate (float x)
{
  // Allows the use of cvttss2si (which produces a signed integer).
  // Correct casting from float to uint32_t goes via the FPU stack in 32-bit mode.
  return (std::uint32_t) (std::int32_t) x;
}

// Convert unsigned int to float (argument's msb must be zero).
ALWAYS_INLINE inline float ui2f (std::uint32_t x)
{
  // Allows the use of cvtsi2ss (which takes a signed integer).
  // Correct casting from uint32_t to float is messy in 32-bit mode.
  return (float) (std::int32_t) x;
}

#endif
