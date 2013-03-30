#include "rodrigues.h"
#include "vector.h"
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
    // f: Remes error +-0x1.950328P-21, max ulp error +-7.
    // g: Remes error +-0x1.4711d2P-24, max ulp error +-2.
    v4f f = { +0x1.ffffe6P-1f, -0x1.55502cP-3f, +0x1.1068aaP-7f, -0x1.847be2P-13f, };
    v4f g = { +0x1.fffffaP-2f, -0x1.555340P-5f, +0x1.6b8f0cP-10f, -0x1.89e392P-16f, };
    v4f x2 = x1 * x1;                  // 1 x^2 1 x^2
    v4f f1 = f * x1;                   // f0 f1x f2 f3x
    v4f g1 = g * x1;                   // g0 g1x g2 g3x
    v4f fg2 = _mm_hadd_ps (f1, g1);    // f0+f1x f2+f3x g0+g1x g2+g3x
    v4f fg3 = fg2 * x2;                // f0+f1x f2x^2+f3x^3 g0+g1x g2x^2+g3x^3
    v4f fgfg = _mm_hadd_ps (fg3, fg3); // f(x) g(x) f(x) g(x)
    return fgfg;
  }

  // Evaluate f and g at xsq.
  // Range [0, 22.206610] ((3pi/2)^2).
  // Argument xsq xsq * *, result sin1(x) cos2(x) * *.
  inline v4f fg (const v4f xsq)
  {
    v4f lim = { +0x1.3bd3ccP1f, 0.0f, 0.0f, 0.0f, }; // (pi/2)^2
    v4f k1 = { 1.0f, 1.0f, 0.0f, 0.0f, };
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 1.
      v4f x1 = _mm_unpacklo_ps (k1, xsq);    // 1 x^2 1 x^2
      return fg_reduced (x1);                // sin1(x) cos2(x) sin1(x) cos2(x)
    }
    else {
      // Quadrants 2 and 3.
      // Use rsqrt and mul to approximate the square root.
      v4f rx = _mm_rsqrt_ss (xsq);           // x^-1 x^2 * *
      v4f xx1 = _mm_unpacklo_ps (xsq, k1);   // x^2 1 * *
      v4f xx = rx * xx1;                     // x x^2 * *
      // Call fg_reduced on (pi-sqrt(xsq))^2.
      v4f kpi = { +0x1.921fb4P1f, 0.0f, 0.0f, 0.0f, }; // pi
      v4f px1 = xx - kpi;                    // x-pi * * *
      v4f px2 = px1 * px1;                   // (pi-x)^2 * * *
      v4f px3 = _mm_unpacklo_ps (k1, px2);   // 1 (pi-x)^2 * *
      v4f px4 = _mm_movelh_ps (px3, px3);    // 1 (pi-x)^2 1 (pi-x)^2
      v4f fg = fg_reduced (px4);             // sin(pi-x)/(pi-x) [1-cos(pi-x)]/(pi-x)^2 * *
      // Recover sin(x) and 1-cos(x) from the result.
      v4f px5 = _mm_unpacklo_ps (px1, px2);  // x-pi (pi-x)^2 * *
      v4f sc1 = px5 * fg;                    // -sin(x) 1+cos(x) * *
      v4f k02 = { 0.0f, 2.0f, 0.0f, 0.0f, };
      v4f sc2 = k02 - sc1;                   // sin(x) 1-cos(x) * *
      // Reciprocal-multiply to approximate f and g.
      return _mm_rcp_ps (xx) * sc2;          // sin1(x) cos2(x) * *
    }
  }

  // g(s) = g0(sqrt(s)) where g0(x)=(x/2)cot(x/2).
  // h(s) = h0(sqrt(s)) where h0(x)=(1-g(x))/x^2.

  // Range [0, (pi/2)^2].
  // Argument 1 s 1 s, result h(s) h(s) h(s) h(s).
  inline v4f h_reduced (const v4f s1)
  {
    // Evaluate polynomial at s by Estrin's method. Requires SSE3 (for haddps).
    // Remes error +-0x1.e7b99eP-28, max ulp error +-7.
    v4f h = { +0x1.555552P-4f, +0x1.6c1cd6P-10f, +0x1.13e3e4P-15f, +0x1.f88a10P-21f, };
    v4f s2 = s1 * s1;              // 1 s^2 1 s^2
    v4f h1 = h * s1;               // h0 h1s h2 h3s
    v4f h2 = _mm_hadd_ps (h1, h1); // h0+h1s h2+h3s h0+h1s h2+h3s
    v4f h3 = h2 * s2;              // h0+h1s h2s^2+h3s^3 h0+h1s h2s^2+h3s^3
    v4f hh = _mm_hadd_ps (h3, h3); // h(s) h(s) h(s) h(s)
    return hh;
  }

  inline v4f tangent (v4f u, v4f w)
  {
    v4f xsq = dot (u, u);
    v4f lim = { +0x1.3bd3ccP1f, 0.0f, 0.0f, 0.0f, }; // (pi/2)^2 0 0 0
    v4f g, h;
    // Evaluate g and h at xsq (range [0, (2pi)^2)).
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 1.
      v4f k1 = { 1.0f, 1.0f, 1.0f, 1.0f, };
      v4f x1 = _mm_unpacklo_ps (k1, xsq);    // 1 x^2 1 x^2
      h = h_reduced (x1);
      g = k1 - xsq * h;
    }
    else {
      // Quadrants 2, 3 and 4.
      v4f khalf = { 0.5f, 0.5f, 0.5f, 0.5f, };
      v4f rx = khalf * _mm_sqrt_ps (xsq);    // x/2 x/2 x/2 x/2
      // Call fg_reduced on (1/2)(pi-sqrt(xsq))^2.
      v4f kpi = { +0x1.921fb4P0f, 0.0f, 0.0f, 0.0f, }; // pi/2
      v4f px1 = rx - kpi;                    // x/2-pi/2 * * *
      v4f px2 = px1 * px1;                   // (pi/2-x/2)^2 * * *
      v4f k1 = { 1.0f, 1.0f, 1.0f, 1.0f, };
      v4f px3 = _mm_unpacklo_ps (k1, px2);   // 1 (pi/2-x/2)^2 * *
      v4f px4 = _mm_movelh_ps (px3, px3);    // 1 (pi/2-x/2)^2 1 (pi/2-x/2)^2
      v4f fg = fg_reduced (px4);             // sin(pi/2-x/2)/(pi/2-x/2) [1-cos(pi/2-x/2)]/(pi/2-x/2)^2 * *
      // Recover sin(x) and cos(x) from the result.
      v4f px5 = _mm_unpacklo_ps (px1, px2);  // x/2-pi/2 (pi/2-x/2)^2 * *
      v4f sc1 = px5 * fg;                    // -cos(x/2) 1-sin(x/2) * *
      v4f k01 = { 0.0f, 1.0f, 0.0f, 0.0f, };
      v4f cs1 = k01 - sc1;                   // cos(x/2) sin(x/2) * *
      v4f cs2 = _mm_unpacklo_ps (cs1, cs1);  // cos(x/2) cos(x/2) sin(x/2) sin(x/2)
      v4f c = _mm_movelh_ps (cs2, cs2);
      v4f s = _mm_movehl_ps (cs2, cs2);
      g = rx * c / s;
      h = (k1 - g) / xsq;
    }
    v4f khalf = { 0.5f, 0.5f, 0.5f, 0.5f, };
    return dot (u, w) * h * u + g * w - khalf * cross (u, w);
  }

  // bch(x,y) = z, where e^\hat{x} e^\hat{y} = e^\hat{z}, for small x.
  inline v4f bch (v4f x, v4f y)
  {
    // One step of the classical fourth-order Runge-Kutta method.
    v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
    v4f A = tangent (y, x);
    v4f B = tangent (y + half * A, x);
    v4f C = tangent (y + half * B, x);
    v4f D = tangent (y + C, x);
    v4f sixth = { 0x1.555554P-3f, 0x1.555554P-3f, 0x1.555554P-3f, 0x1.555554P-3f, };
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
  for (unsigned n = 0; n != count; ++ n) {
    v4f u1 = bch (dt0 * load4f (w [n]), load4f (u [n]));
    v4f xsq = dot (u1, u1);
    v4f klim1 = { +0x1.634e46P4f, 0.0f, 0.0f, 0.0f, }; // (3pi/2)^2
    if (_mm_comigt_ss (xsq, klim1)) {
      v4f k1 = { 1.0f, 1.0f, 1.0f, 1.0f, };
      v4f k2pi = { 0x1.921fb4P2f, 0x1.921fb4P2f, 0x1.921fb4P2f, 0x1.921fb4P2f, };
      u1 *= k1 - k2pi / _mm_sqrt_ps (xsq);
    }
    store4f (u [n], u1);
  }
}

void compute (float (* f) [16], const float (* x) [4], const float (* u) [4], unsigned count)
{
  for (unsigned n = 0; n != count; ++ n) {
    v4f u4 = load4f (u [n]);           // u0 u1 u2 0
    v4f usq = u4 * u4;                 // u0^2 u1^2 u2^2 0
    v4f uha = _mm_hadd_ps (usq, usq);
    v4f xsq = _mm_hadd_ps (uha, uha);  // x^2 x^2 x^2 x^2 (x = length of u)
    v4f ab = fg (xsq);
    v4f a = broadcast0 (ab);
    v4f b = broadcast1 (ab);
    v4f skew = a * u4;
    v4f u1 = _mm_shuffle_ps (u4, u4, SHUFFLE (1, 2, 0, 3)); // u1 u2 u0 u3
    v4f u2 = _mm_shuffle_ps (u4, u4, SHUFFLE (2, 0, 1, 3)); // u2 u0 u1 u3
    v4f symm = b * (u1 * u2);
    v4f sub = symm - skew;                       // s0 s1 s2 0
    v4f add = symm + skew;                       // a0 a1 a2 0
    v4f usql = _mm_movelh_ps (usq, usq);         // u0^2 u1^2 u0^2 u1^2
    v4f usqd = _mm_moveldup_ps (usq);            // u0^2 u0^2 u2^2 u2^2
    v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
    v4f diag = one - b * (usql + usqd);          // * d2 d1 d0
    v4f aslo = _mm_movelh_ps (add, sub);         // a0 a1 s0 s1
    v4f ashi = _mm_unpackhi_ps (add, sub);       // a2 s2 0 0
    v4f ashj = _mm_movelh_ps (ashi, ashi);       // a2 s2 a2 s2
    v4f ashd = _mm_movehl_ps (diag, ashj);       // a2 s2 d1 d0
    v4f das = _mm_shuffle_ps (ashd, sub, SHUFFLE (3, 0, 1, 3));  // d0 a2 s1 0
    v4f sda = _mm_shuffle_ps (ashd, add, SHUFFLE (1, 2, 0, 3));  // s2 d1 a0 0
    v4f asd = _mm_shuffle_ps (aslo, diag, SHUFFLE (1, 2, 1, 0)); // a1 s0 d2 *
    store4f (& f [n] [0], das);
    store4f (& f [n] [4], sda);
    store4f (& f [n] [8], asd);
    f [n] [11] = 0.0f;
    store4f (& f [n] [12], load4f (x [n]));
    f [n] [15] = 1.0f;
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
  v4f a = broadcast0 (ab);
  v4f b = broadcast1 (ab);
  v4f v0 = load4f (v);
  v4f v1 = v0 + a * cross (u1, v0) + b * (dot (u1, v0) * u1 - xsq * v0);
  // To save code don't worry about u growing too large (compare advance_angular).
  store4f (u, bch (v1, u1));
}
