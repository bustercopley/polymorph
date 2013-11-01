#include "rodrigues.h"
#include "compiler.h"

// See `problem.tex' for terminology and notation, and `problem.tex',
// `script.mathomatic' and `script.maxima' for derivations.

// `advance_linear' and `advance_angular' update the linear and the
// angular position, x and u, for inertial motion with constant linear
// velocity v and angular velocity w over the time increment dt.
// It uses a single step of the classical fourth order Runge-Kutta
// method to integrate the differential equation from `problem.tex'.

// `compute' updates the OpenGL matrix from the linear and angular
// position vectors x and u. It is implemented using SSE3 intrinsics.

// Approximations: for the functions
//   f(x^2)=sin(x)/x,
//   g(x^2)=(1-cos(x))/x^2,
//   h(x^2)=(1-((x/2)*cot(x/2)))/x^2,
// the minimax polynomials (whose maximum absolute error over the
// specified range is minimal among polynomials of the same degree,
// according to the Chebychev equioscillation theorem) were calculated
// at excess precision by the Remes algorithm.

// The max ulp error quoted is the absolute difference from the exact
// value correctly rounded toward zero, and is the maximum over 600000
// equally spaced sample arguments x^2.

// Helpers for `compute' and `advance_angular'.
namespace
{
  // f(s)=sin1(sqrt(s)), where sin1(x)=sin(x)/x.
  // g(s)=cos2(sqrt(s)), where cos2(x)=(1-cos(x))/x^2.

  // Range [0, 2.467401] ((pi/2)^2).
  // Argument 1 xsq 1 xsq, result sin1(x) cos2(x) sin1(x) cos2(x).
  inline v4f fg_reduced (const v4f x1)
  {
    // Evaluate two polynomials at x by Estrin's method. Requires SSE3 (for haddps).
    // f: Remes error +-0x1.950326P-21, max ulp error +-7.
    // g: Remes error +-0x1.4711d0P-24, max ulp error +-2.
    v4f f = { +0x1.ffffe6P-1f, -0x1.55502cP-3f, +0x1.1068acP-7f, -0x1.847be2P-13f, };
    v4f g = { +0x1.fffffaP-2f, -0x1.555340P-5f, +0x1.6b8f0cP-10f, -0x1.89e394P-16f, };
    v4f fx1 = f * x1;                  // f0 f1x f2 f3x
    v4f gx1 = g * x1;                  // g0 g1x g2 g3x
    v4f fg2 = _mm_hadd_ps (fx1, gx1);  // f0+f1x f2+f3x g0+g1x g2+g3x
    v4f fg3 = fg2 * x1 * x1;           // f0+f1x f2x^2+f3x^3 g0+g1x g2x^2+g3x^3
    v4f fgfg = _mm_hadd_ps (fg3, fg3); // f(x) g(x) f(x) g(x)
    return fgfg;
  }

  // Evaluate f and g at xsq.
  // Range [0, 22.206610] (((3/2)*pi)^2).
  // Argument xsq xsq * *, result sin1(x) cos2(x) sin1(x) cos2(x).
  inline v4f fg (const v4f xsq)
  {
    v4f lim = { +0x1.3bd3ccP1f, 0.0f, 0.0f, 0.0f, }; // (pi/2)^2
    v4f one = { 1.0f, 1.0f, 0.0f, 0.0f, };
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 1.
      v4f x1 = _mm_unpacklo_ps (one, xsq);   // 1 x^2 1 x^2
      return fg_reduced (x1);                // sin1(x) cos2(x) sin1(x) cos2(x)
    }
    else {
      // Quadrants 2 and 3.
      v4f x = sqrt (xsq);                    // x x * * (approx)
      // Call fg_reduced on (pi-x)^2.
      v4f pi = { +0x1.921fb6P1f, +0x1.921fb6P1f, 0.0f, 0.0f, }; // pi pi 0 0
      v4f px1 = x - pi;                      // x-pi x-pi * *
      v4f px2 = px1 * px1;                   // (pi-x)^2 (pi-x)^2 * *
      v4f px3 = _mm_unpacklo_ps (one, px2);  // 1 (pi-x)^2 1 (pi-x)^2
      v4f fgfg = fg_reduced (px3);           // sin(pi-x)/(pi-x) [1-cos(pi-x)]/(pi-x)^2 (duplicated)
      // Recover sin(x) and 1-cos(x) from the result.
      v4f px4 = _mm_unpacklo_ps (px1, px2);  // x-pi (pi-x)^2 x-pi (pi-x)^2
      v4f sc1 = px4 * fgfg;                  // -sin(x) 1+cos(x) -sin(x) 1+cos(x)
      v4f k02 = { 0.0f, 2.0f, 0.0f, 2.0f, };
      v4f sc2 = k02 - sc1;                   // sin(x) 1-cos(x) sin(x) 1-cos(x)
      // Reciprocal-multiply to approximate f and g.
      v4f xx = _mm_unpacklo_ps (x, xsq);     // x x^2 x x^2
      return rcp (xx) * sc2;                 // sin1(x) cos2(x) sin1(x) cos2(x)
    }
  }

  // g(s) = g0(sqrt(s)) where g0(x)=(x/2)cot(x/2).
  // h(s) = h0(sqrt(s)) where h0(x)=(1-g(x))/x^2.

  // Range [0, (pi/2)^2].
  // Argument 1 s 1 s, result h(s) h(s) h(s) h(s).
  inline v4f h_reduced (const v4f s1)
  {
    // Evaluate polynomial at s by Estrin's method. Requires SSE3 (for haddps).
    // Remes error +-0x1.e7b99cP-28, max ulp error +-2.
    v4f h = { +0x1.555554P-4f, +0x1.6c1cd6P-10f, +0x1.13e3e4P-15f, +0x1.f88a10P-21f, };
    v4f h1 = h * s1;               // h0 h1s h2 h3s
    v4f h2 = _mm_hadd_ps (h1, h1); // h0+h1s h2+h3s h0+h1s h2+h3s
    v4f h3 = h2 * s1 * s1;         // h0+h1s h2s^2+h3s^3 h0+h1s h2s^2+h3s^3
    v4f hh = _mm_hadd_ps (h3, h3); // h(s) h(s) h(s) h(s)
    return hh;
  }

  inline v4f tangent (v4f u, v4f w)
  {
    v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
    v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
    v4f lim = { +0x1.3bd3ccP1f, 0.0f, 0.0f, 0.0f, }; // (pi/2)^2 0 0 0
    v4f xsq = dot (u, u);
    v4f g, h;
    // Evaluate g and h at xsq (range [0, (2pi)^2)).
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 1.
      v4f x1 = _mm_unpacklo_ps (one, xsq);   // 1 x^2 1 x^2
      h = h_reduced (x1);
      g = one - xsq * h;
    }
    else {
      // Quadrants 2, 3 and 4.
      v4f hx = half * sqrt (xsq);            // x/2 x/2 x/2 x/2 (approx)
      // Call fg_reduced on (1/2)(pi-sqrt(xsq))^2.
      v4f pi = { +0x1.921fb6P0f, +0x1.921fb6P0f, 0.0f, 0.0f, }; // pi/2 pi/2 * *
      v4f px1 = hx - pi;                     // x/2-pi/2 x/2-pi/2 * *
      v4f px2 = px1 * px1;                   // (pi/2-x/2)^2 (pi/2-x/2)^2 * *
      v4f px3 = _mm_unpacklo_ps (one, px2);  // 1 (pi/2-x/2)^2 1 (pi/2-x/2)^2
      v4f fgfg = fg_reduced (px3);           // sin(pi/2-x/2)/(pi/2-x/2) [1-cos(pi/2-x/2)]/(pi/2-x/2)^2 (duplicated)
      // Recover sin(x/2) and cos(x/2) from the result.
      v4f px4 = _mm_unpacklo_ps (px1, px2);  // x/2-pi/2 (pi/2-x/2)^2 x/2-pi/2 (pi/2-x/2)^2
      v4f sc = px4 * fgfg;                   // -cos(x/2) 1-sin(x/2) -cos(x/2) 1-sin(x/2)
      v4f k01 = { 0.0f, 1.0f, 0.0f, 1.0f, };
      v4f cs = k01 - sc;                     // cos(x/2) sin(x/2) cos(x/2) sin(x/2)
      v4f c = _mm_moveldup_ps (cs);
      v4f s = _mm_movehdup_ps (cs);
      g = hx * c * rcp (s);
      h = (one - g) * rcp (xsq);
    }
    return dot (u, w) * h * u + g * w - half * cross (u, w);
  }

  // bch(x,y) = z, where e^\hat{x} e^\hat{y} = e^\hat{z}, for small x.
  inline v4f bch (v4f x, v4f y)
  {
    // One step of the classical fourth-order Runge-Kutta method.
    v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
    v4f sixth = { 0x1.555556P-3f, 0x1.555556P-3f, 0x1.5555556P-3f, 0x1.555556P-3f, };
    v4f A = tangent (y, x);
    v4f B = tangent (y + half * A, x);
    v4f C = tangent (y + half * B, x);
    v4f D = tangent (y + C, x);
    return y + sixth * (A + (B + C) + (B + C) + D);
  }
}

void advance_linear (float (* x) [4], const float (* v) [4], unsigned count, float dt)
{
  // Load 32 bytes (one cache line) of data at a time.
  // This can operate on padding at the end of the arrays.
  v4f dt0 = _mm_set1_ps (dt);
  for (unsigned n = 0; n != (count + 1) >> 1; ++ n) {
    v4f x0 = load4f (x [n << 1]);
    v4f x1 = load4f (x [(n << 1) | 1]);
    v4f v0 = load4f (v [n << 1]);
    v4f v1 = load4f (v [(n << 1) | 1]);
    v4f x10 = x0 + v0 * dt0;
    v4f x11 = x1 + v1 * dt0;
    store4f (x [n << 1], x10);
    store4f (x [(n << 1) | 1], x11);
  }
}

void advance_angular (float (* u) [4], float (* w) [4], unsigned count, float dt)
{
  v4f dt0 = _mm_set1_ps (dt);
  v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
  v4f twopi = { 0x1.921fb6P2f, 0x1.921fb6P2f, 0x1.921fb6P2f, 0x1.921fb6P2f, };
  v4f lim1 = { +0x1.3c0000P3f, 0.0f, 0.0f, 0.0f, }; // a touch over pi^2 (~ +0x1.3bd3ccP3)
  for (unsigned n = 0; n != count; ++ n) {
    v4f u1 = bch (dt0 * load4f (w [n]), load4f (u [n]));
    v4f xsq = dot (u1, u1);
    if (_mm_comigt_ss (xsq, lim1)) {
      u1 *= one - twopi * rsqrt (xsq);
    }
    store4f (u [n], u1);
  }
}

void compute (char * buffer, std::size_t stride, const float (* x) [4], const float (* u) [4], unsigned count)
{
#if __SSE4_1__
#else
  __m128i iv = { 0, -0x100000000ll, };
  v4f mask = _mm_castsi128_ps (iv);    // false false false true
#endif
  char * iter = buffer;
  for (unsigned n = 0; n != count; ++ n, iter += stride) {
    float (& f) [16] = * reinterpret_cast <float (*) [16]> (iter);
    v4f u0 = load4f (u [n]);           // u0 u1 u2 0
    v4f usq = u0 * u0;                 // u0^2 u1^2 u2^2 0
    v4f uha = _mm_hadd_ps (usq, usq);
    v4f xsq = _mm_hadd_ps (uha, uha);  // x^2 x^2 x^2 x^2 (x = length of u)
    v4f ab = fg (xsq);
    v4f a = _mm_moveldup_ps (ab);
    v4f b = _mm_movehdup_ps (ab);
    v4f skew = a * u0;
    v4f u1 = _mm_shuffle_ps (u0, u0, SHUFFLE (1, 2, 0, 3)); // u1 u2 u0 0
    v4f u2 = _mm_shuffle_ps (u0, u0, SHUFFLE (2, 0, 1, 3)); // u2 u0 u1 0
    v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
    v4f symm = b * (u1 * u2);
    v4f sub = symm - skew;                       // s0 s1 s2  0
    v4f add = symm + skew;                       // a0 a1 a2  0
    v4f phicos = one + b * (usq - xsq);          // d0 d1 d2 cos(x)
    v4f aslo = _mm_movelh_ps (add, sub);         // a0 a1 s0 s1
    v4f ashi = _mm_unpackhi_ps (add, sub);       // a2 s2  0  0
    v4f ashd = _mm_movelh_ps (ashi, phicos);     // a2 s2 d0 d1
    v4f xyz0 = load4f (x [n]);
#if __SSE4_1__
    v4f phi = _mm_blend_ps (phicos, add, 8);     // d0 d1 d2  0
    v4f xyz1 = _mm_blend_ps (xyz0, one, 8);      // x0 x1 x2  1
#else
    v4f phi = _mm_andnot_ps (mask, phicos);
    v4f xyz1 = _mm_or_ps (_mm_andnot_ps (mask, xyz0), _mm_and_ps (mask, one));
#endif
    store4f (& f [0], _mm_shuffle_ps (ashd, sub, SHUFFLE (2, 0, 1, 3)));  // d0 a2 s1 0
    store4f (& f [4], _mm_shuffle_ps (ashd, add, SHUFFLE (1, 3, 0, 3)));  // s2 d1 a0 0
    store4f (& f [8], _mm_shuffle_ps (aslo, phi, SHUFFLE (1, 2, 2, 3)));  // a1 s0 d2 0
    store4f (& f [12], xyz1);                                             // x0 x1 x2 1
  }
}

// bch(u,v) for small v (roughly, |v| <= pi/4).
void rotate (float (& u) [4], const float (& v) [4])
{
  // The integrator is designed to compute bch(x,y) for small x; here we want bch(u,v)
  // where only v is required to be relatively small. We make use of the formula
  // bch(u,v) = bch(e^\hat{u}v,u), which is a corollary of Ad(e^x)=e^ad(x).
  // Compute v1 = e^\hat{u}v using Rodrigues' formula.
  v4f u1 = load4f (u);
  v4f xsq = dot (u1, u1);
  v4f ab = fg (xsq);
  v4f a = _mm_moveldup_ps (ab);
  v4f b = _mm_movehdup_ps (ab);
  v4f v0 = load4f (v);
  v4f v1 = v0 + a * cross (u1, v0) + b * (dot (u1, v0) * u1 - xsq * v0);
  // To save code don't worry about u growing too large (compare advance_angular).
  store4f (u, bch (v1, u1));
}

// Restricted range [-pi/2, pi/2].
// Argument x x * *, result sin(x) cos(x) sin(x) cos(x).
v4f sincos (const v4f x)
{
  v4f one = {1.0f,  1.0f, 1.0f, 1.0f, };
  v4f alt = {0.0f,  1.0f, 0.0f, 1.0f, };
  v4f xsq = x * x;                       // x^2 x^2 * *
  v4f x1 = _mm_unpacklo_ps (one, xsq);   // 1 x^2 1 x^2
  v4f fg = fg_reduced (x1);              // sin1(x) cos2(x) sin1(x) cos2(x)
  v4f x2 = _mm_unpacklo_ps (-x, xsq);    // -x x^2 -x x^2
  return alt - fg * x2;                  // sin(x) cos(x) sin(x) cos(x)
}

// Very restricted range [+0x1.8c97f0P-1f, +0x1.fb5486P-1f] ([0.774596691, 0.990879238]).
// Argument x x x x, result acos(x) acos(x) acos(x) acos(x).
v4f arccos (v4f x)
{
  // Minimax polynomial for (acos(x))^2 on [+0x1.8c97f0P-1f, +0x1.fb5486P-1f].
  // Remes error +-0x1.460d54P-21f.
  v4f one = {1.0f,  1.0f, 1.0f, 1.0f, };
  v4f c = { +0x1.37b24aP1f, -0x1.7cb23cP1f, +0x1.494690P-1f, -0x1.aa37e2P-4f, };
  v4f x1 = _mm_unpacklo_ps (one, x);       // 1 x 1 x
  v4f c1 = c * x1;                         // c0 c1x c2 c3x
  v4f c2 = _mm_hadd_ps (c1, c1);           // c0+c1x c2+c3x c0+c1x c2+c3x
  v4f c3 = c2 * x1 * x1;                   // c0+c1x c2x^2+c3x^3 c0+c1x c2x^2+c3x^3
  v4f cc = _mm_hadd_ps (c3, c3);           // c(x) c(x) c(x) c(x)
  return sqrt (cc);
}
