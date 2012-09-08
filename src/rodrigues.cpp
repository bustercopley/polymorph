#include "rodrigues.h"
#include "vector.h"
#include <cmath>

// See `problem.tex' for terminology and notation, and `problem.tex',
// `script.mathomatic' and `script.maxima' for derivations.

// `advance' updates the linear and angular position x and u for
// inertial motion with constant linear velocity v and angular
// velocity w over the time increment dt. It uses a single step
// of the classical fourth order Runge-Kutta method to integrate
// the differential equation from `problem.tex'.

// `compute' updates the OpenGL matrix from the linear and angular
// position vectors x and u. It is implemented using SSE3 intrinsics.

// Approximations: the polynomials minimizing the maximum absolute error
// according to the Chebychev equioscillation theorem were calculated
// by the Remes algorithm at excess precision, for the functions
//   f(x^2)=sin(x)/x,
//   g(x^2)=(1-cos(x))/x^2,
//   h(x^2)=(1-((x/2)*cot(x/2)))/x^2.
// The quoted max ulp error quoted is the absolute difference from the
// exact value correctly rounded toward zero, and is the maximum over
// 600000 equally spaced sample arguments x^2.

// Helpers for `advance'.
namespace
{
  // h(x^2)=(1-((x/2)*cot(x/2)))/x^2
  // Range [0, +0x1.18bc4418cafe2P-2] ((pi/6)^2)
  // Remes error +-0x1.d6715eac092e5P-50, max ulp error +-119.
  inline double h_reduced (double xsq)
  {
    return (+0x1.55555555555caP-4 +
            (+0x1.6c16c16ac8123P-10 +
             (+0x1.1566b082f0c0dP-15 +
              (+0x1.bbcb57ee3f61fP-21 +
               (+0x1.6cf48215f69b3P-26) * xsq) * xsq) * xsq) * xsq);
  }

  void tangent (const double (& u) [4], const float (& w) [4], double (& udot) [4])
  {
    double g, h;
    v2d xsqv = dot (u, u);
    v2d lim = { 0x1.18bc4418cafe2P-2, 0x1.18bc4418cafe2P-2, };
    if (_mm_comigt_sd (xsqv, lim)) {
      double xxsq [2]; // x xsq
      _mm_store_pd (xxsq, _mm_sqrt_sd (xsqv, xsqv));
      double halfx = 0.5 * xxsq [0];
      double halfpi = 0x1.921fb54442d18P0;
      g = halfx * std::tan (halfpi - halfx);
      h = (1 - g) / xxsq [1];
    }
    else {
      double xsq = _mm_cvtsd_f64 (xsqv);
      h = h_reduced (xsq);
      g = 1 - xsq * h; // Using the degree-4 Remes h, the max ulp error in g is +-1.
    }
    v4f u0 = _mm_cvtpd_ps (_mm_load_pd (& u [0]));
    v4f u1 = _mm_cvtpd_ps (_mm_load_pd (& u [2]));
    float uw = _mm_cvtss_f32 (dot (_mm_movelh_ps (u0, u1), load4f (w)));
    double ch = uw * h;
    VECTOR_ijk udot [i] = ch * u [i] + g * w [i] - (u [j] * w [k] - u [k] * w [j]) / 2;
  }

  extern inline void rk4 (double (& u) [4], const float (& w) [4], float dt)
  {
    double t [4], A [4], B [4], C [4], D [4];
    tangent (u, w, A);
    VECTOR t [i] = u [i] + 0.5 * dt * A [i];
    tangent (t, w, B);
    VECTOR t [i] = u [i] + 0.5 * dt * B [i];
    tangent (t, w, C);
    VECTOR t [i] = u [i] + dt * C [i];
    tangent (t, w, D);
    VECTOR u [i] += (dt / 6) * (A [i] + 2 * (B [i] + C [i]) + D [i]);
  }

  // It would probably be overkill to use the following degree-5 approximation for h.
  // Remes error +-0x1.a38da78a2d608P-59, max ulp error +-1.
  // double c [5 + 1] = {
  //   +0x1.5555555555555P-4,
  //   +0x1.6c16c16c17986P-10,
  //   +0x1.1566abbb9b596P-15,
  //   +0x1.bbd78a875d2c8P-21,
  //   +0x1.6699bf709a016P-26,
  //   +0x1.28a24352ce94dP-31,
  // };
}

void advance_linear (float (* x) [4], const float (* v) [4], unsigned count, float dt)
{
  // This can operate on padding at the end of the arrays.
  v4f xdt = _mm_set1_ps (dt);
  for (unsigned n = 0; n != (count + 1) >> 1; ++ n) {
    v4f xx0 = load4f (x [n << 1]);
    v4f xx1 = load4f (x [(n << 1) | 1]);
    v4f xv0 = load4f (v [n << 1]);
    v4f xv1 = load4f (v [(n << 1) | 1]);
    v4f xx10 = xx0 + xv0 * xdt;
    v4f xx11 = xx1 + xv1 * xdt;
    store4f (x [n << 1], xx10);
    store4f (x [(n << 1) | 1], xx11);
  }
}

void advance_angular (double (* u) [4], float (* w) [4], unsigned count, float dt)
{
  for (unsigned n = 0; n != count; ++ n) {
    rk4 (u [n], w [n], dt);
    double xsq = _mm_cvtsd_f64 (dot (u [n], u [n]));
    if (xsq > 10.0) {
      double m = 1.0 - 2 * 0x1.921fb54442d18P1 / std::sqrt (xsq); // pi
      VECTOR u [n] [i] *= m;
    }
  }
}

// Helpers for `compute'.
namespace
{
  // f(x) = (sin(sqrt(x)))/sqrt(x)
  // g(x) = (1-cos(sqrt(x)))/x

  // Evaluate two polynomials at x by Estrin's method. Requires SSE3 (for haddps).
  // Range [0, 2.467401] ((pi/2)^2).
  // Argument 1 xsq 1 xsq, result sin1(x) cos2(x) sin1(x) cos2(x).
  extern inline v4f fg_reduced (const v4f x1)
  {
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
  extern inline v4f fg (const v4f xsq)
  {
    v4f lim = { +0x1.3bd3ccP1f, 0.0f, 0.0f, 0.0f, }; // (pi/2)^2
    v4f k1 = { 1.0f, 1.0f, 0.0f, 0.0f, };
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 0.
      v4f x1 = _mm_unpacklo_ps (k1, xsq);    // 1 x^2 1 x^2
      return fg_reduced (x1);                   // sin1(x) cos2(x) sin1(x) cos2(x)
    }
    else {
      // Quadrants 1 and 2.
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
}

void compute (float (& f) [16], const float (& x) [4], const double (& u) [4], float r)
{
  v4f u01 = _mm_cvtpd_ps (_mm_loadu_pd (& u [0]));
  v4f u23 = _mm_cvtpd_ps (_mm_loadu_pd (& u [2]));
  v4f u4 = _mm_movelh_ps (u01, u23); // u0 u1 u2 0
  v4f usq = u4 * u4;                 // u0^2 u1^2 u2^2 0
  v4f uha = _mm_hadd_ps (usq, usq);
  v4f xsq = _mm_hadd_ps (uha, uha);  // x^2 x^2 x^2 x^2 (x = length of u)
  v4f rr = _mm_set1_ps (r);
  float rab [4];
  store4f (rab, rr * fg (xsq));
  v4f ra = _mm_set1_ps (rab [0]);
  v4f rb = _mm_set1_ps (rab [1]);
  v4f skew = ra * u4;
  v4f u1 = _mm_shuffle_ps (u4, u4, SHUFFLE (1, 2, 0, 3)); // u1 u2 u0 u3
  v4f u2 = _mm_shuffle_ps (u4, u4, SHUFFLE (2, 0, 1, 3)); // u2 u0 u1 u3
  v4f symm = rb * (u1 * u2);
  v4f sub = symm - skew;                       // s0 s1 s2 0
  v4f add = symm + skew;                       // a0 a1 a2 0
  v4f usql = _mm_movelh_ps (usq, usq);         // u0^2 u1^2 u0^2 u1^2
  v4f usqd = _mm_moveldup_ps (usq);            // u0^2 u0^2 u2^2 u2^2
  v4f rbusqld = rb * (usql + usqd);
  v4f diag = rr - rbusqld;                     // * d2 d1 d0
  v4f aslo = _mm_movelh_ps (add, sub);         // a0 a1 s0 s1
  v4f ashi = _mm_unpackhi_ps (add, sub);       // a2 s2 0 0
  v4f ashj = _mm_movelh_ps (ashi, ashi);       // a2 s2 a2 s2
  v4f ashd = _mm_movehl_ps (diag, ashj);       // a2 s2 d1 d0
  v4f das = _mm_shuffle_ps (ashd, sub, SHUFFLE (3, 0, 1, 3));  // d0 a2 s1 0
  v4f sda = _mm_shuffle_ps (ashd, add, SHUFFLE (1, 2, 0, 3));  // s2 d1 a0 0
  v4f asd = _mm_shuffle_ps (aslo, diag, SHUFFLE (1, 2, 1, 0)); // a1 s0 d2 *
  store4f (& f [0], das);
  store4f (& f [4], sda);
  store4f (& f [8], asd);
  f [11] = 0.0f;
  store4f (& f [12], load4f (x));
  f [15] = 1.0f;
}
