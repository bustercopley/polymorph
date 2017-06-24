// Copyright 2012-2017 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mswin.h"
#include "rodrigues.h"
#include "compiler.h"

// The function "compute" implements the axis-angle representation.
// It takes three-dimensional vectors v and returns 3x3 matrices M.
// Here M is a rotation matrix. The axis of the rotation is the line
// through the origin in the direction of v. The rotation is through
// an angle equal to the length of v, in radians. The sense of the
// rotation is given by the right hand rule.

// The functions "bch2" and "bch4" take two vectors representing
// rotations, and return a vector representing the composition of
// those two rotations. In other words, for any vectors u and v,
// the rotation matrix "compute(bch(u,v))" is the product of the
// matrices "compute(u)" and "compute(v)".

// There do exist closed-form formulae to combine two axis-angle
// representations, but as far as I know there is none that is
// defined on the whole space of rotations.

// Instead we set up a differential equation, described by the
// "tangent" function, and use numerical methods to solve it in the
// functions "bch2" and "bch4"; as a result, the "bch" functions
// combine two rotations provided one of the rotations is
// "sufficiently small".

// It's common practice to represent a rotation by a unit quaternion,
// and for good reason: combining rotations is much simpler and more
// efficient and ultimately easier to understand in that system.
// On the other hand, applying an external torque to an angular
// momentum is easier in the axis-angle representation (see
// bounce.h, but don't expect lucid exposition.)

// Implementation.

// Approximations: for the functions
//   f(x^2)=f0(x)=sin(x)/x,
//   g(x^2)=g0(x)=(1-cos(x))/x^2,
//   h(x^2)=h0(x)=(1-k0(x))/x^2, where k0(x)=(x/2)*cot(x/2),
// the minimax polynomials (whose maximum absolute error over the
// specified range is minimal among polynomials of the same degree,
// according to the Chebychev equioscillation theorem) were calculated
// at excess precision by the Remes algorithm.

// The max ulp error quoted is the absolute difference from the exact
// value correctly rounded toward zero, and is the maximum over 600000
// equally spaced sample arguments x^2.

// Helpers for "compute" and "advance_angular".
namespace
{
  // Range [0, 2.467401] ((pi/2)^2).
  // f: Remes error +-0x1.950326P-021f, max ulp error +-14.
  // g: Remes error +-0x1.4711d0P-024f, max ulp error +-4.
  // h: Remes error +-0x1.e7b99cP-028f, max ulp error +-2.
  const v4f fpoly = { +0x1.ffffe6P-001f, -0x1.55502cP-003f, +0x1.1068acP-007f, -0x1.847be2P-013f, };
  const v4f gpoly = { +0x1.fffffaP-002f, -0x1.555340P-005f, +0x1.6b8f0cP-010f, -0x1.89e394P-016f, };
  const v4f hpoly = { +0x1.555554P-004f, +0x1.6c1cd6P-010f, +0x1.13e3e4P-015f, +0x1.f88a10P-021f, };

  // Minimax polynomial for (acos(x))^2.
  // Very restricted range [+0x1.8c97f0P-001f, +0x1.fb5486P-001f] ([0.774596691, 0.990879238]).
  // Remes error +-0x1.460d54P-021f, max ulp error +-446.
  const v4f apoly = { +0x1.37b24aP+001f, -0x1.7cb23cP+001f, +0x1.494690P-001f, -0x1.aa37e2P-004f, };

  // Argument x x * *, result sin(x) 1-cos(x) sin(x) 1-cos(x).
  // Range [-pi/2, pi/2].
  inline v4f sincos_internal (const v4f x)
  {
    v4f xsq = x * x;                           // x^2 x^2 * *
    v4f fgfg = polyeval (xsq, fpoly, gpoly);   // sin(x)/x (1-cos(x))/x^2 sin(x)/x (1-cos(x))/x^2
    v4f xmix = _mm_unpacklo_ps (x, xsq);       // x x^2 x x^2
    return xmix * fgfg;                        // sin(x) 1-cos(x) sin(x) 1-cos(x)
  }

  // Argument x y z w, result x-pi y-pi z-pi w-pi.
  inline v4f subtract_pi (v4f x)
  {
    // To 48 binary digits, pi is 1.921fb54442d2P+001.
    v4f pi_hi = { 0x1.921fb4P+001f, 0x1.921fb4P+001f, 0x1.921fb4P+001f, 0x1.921fb4P+001f, };
    v4f pi_lo = { 0x1.4442d2P-023f, 0x1.4442d2P-023f, 0x1.4442d2P-023f, 0x1.4442d2P-023f, };
    return (x - pi_hi) - pi_lo; // I'm pretty sure
  }

  // Evaluate f and g at xsq.
  // Range [0, ((3/2)*pi)^2].
  // Argument x^2 x^2 * *, result f0(x) g0(x) f0(x) g0(x).
  inline v4f fg (const v4f xsq)
  {
    v4f lim = { +0x1.3bd3ccP+001f, 0.0f, 0.0f, 0.0f, }; // (pi/2)^2
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 1 (0 <= x < pi/2).
      return polyeval (xsq, fpoly, gpoly);
    }
    else {
      // Quadrants 2 and 3 (pi/2 <= x < 3pi/2).
      v4f x = sqrt_nonzero (xsq);            // x x * * (approx)
      // Let t = x - pi. Then sin(t) = -sin(x) and cos(t) = -cos(x).
      v4f xmpi = subtract_pi (x);            // x-pi x-pi * *
      v4f sc1 = sincos_internal (xmpi);      // -sin(x) 1+cos(x) -sin(x) 1+cos(x)
      v4f o2o2 = { 0.0f, 2.0f, 0.0f, 2.0f, };
      v4f sc2 = o2o2 - sc1;                  // sin(x) 1-cos(x) sin(x) 1-cos(x)
      // Reciprocal-multiply to approximate f and g.
      v4f xx = _mm_unpacklo_ps (x, xsq);     // x x^2 x x^2
      return rcp (xx) * sc2;                 // f0(x) g0(x) f0(x) g0(x)
    }
  }

  // Total derivative of f(t) at 0, where f is the continuous function R->R^3
  // such that f(0) = u and for all real t, e^{\hat{f}(t)} = e^{\hat{u}} e^{t\hat{w}}.
  inline v4f tangent (v4f u, v4f w)
  {
    v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
    v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
    v4f lim = { +0x1.3bd3ccP+001f, 0.0f, 0.0f, 0.0f, };           // (pi/2)^2 0 0 0
    v4f xsq = dot (u, u);
    v4f g, h;
    // Evaluate g and h at xsq (range [0, (2pi)^2)).
    if (_mm_comile_ss (xsq, lim)) {
      // Quadrant 1 (0 <= x < pi/2).
      h = polyeval (xsq, hpoly, hpoly);  // Cubic in xsq.
      g = one - xsq * h;                 // Quartic in xsq (the extra precision is important).
    }
    else {
      // Quadrants 2, 3 and 4 (pi/2 <= x < 2pi).
      // Let t = x/2 - pi/2. Then sin(t) = -cos(x/2) and cos(t) = sin(x/2).
      v4f x = sqrt_nonzero (xsq);              // x/2 x/2 x/2 x/2 (approx)
      v4f hxmpi = half * subtract_pi (x);      // 0.5*(x-pi)
      v4f sc = sincos_internal (hxmpi);        // -cos(x/2) 1-sin(x/2) -cos(x/2) 1-sin(x/2)
      v4f oioi = { 0.0f, 1.0f, 0.0f, 1.0f, };
      v4f cs = oioi - sc;                      // cos(x/2) sin(x/2) cos(x/2) sin(x/2)
      v4f c = _mm_moveldup_ps (cs);
      v4f s = _mm_movehdup_ps (cs);
      // Reciprocal-multiply to approximate (x/2)cot(x/2).
      g = half * x * c * rcp (s);
      h = (one - g) * rcp (xsq);
    }
    return dot (u, w) * h * u + g * w - half * cross (u, w);
  }

  // Compute z = bch(x,y), where e^\hat{x} e^\hat{y} = e^\hat{z}, for fairly small x.
  inline v4f bch4 (v4f x, v4f y)
  {
    // One step of the classical fourth-order Runge-Kutta method.
    v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
    v4f sixth = { 0x1.555556P-003f, 0x1.555556P-003f, 0x1.555556P-003f, 0x1.555556P-003f, };
    v4f A = tangent (y, x);
    v4f B = tangent (y + half * A, x);
    v4f C = tangent (y + half * B, x);
    v4f D = tangent (y + C, x);
    return y + sixth * ((A + D) + ((B + C) + (B + C)));
  }

  // Compute z = bch(x,y) for very small x.
  inline v4f bch2 (v4f x, v4f y)
  {
    v4f half = { 0.5f, 0.5f, 0.5f, 0.5f, };
    if (1) {
      // One step of the midpoint method.
      v4f a = tangent (y, x);
      v4f b = tangent (y + half * a, x);
      return y + b;
    }
    else {
      // One step of Heun's method.
      v4f a = tangent (y, x);
      v4f b = tangent (y + a, x);
      return y + half * (a + b);
    }
  }
}

// Update position x for motion with constant velocity v over a unit time interval.
void advance_linear (float (* RESTRICT x) [4], const float (* RESTRICT v) [4], unsigned count)
{
  // Load 64 bytes (one cache line) of data at a time.
  // This can operate on padding at the end of the arrays.
  for (unsigned n = 0; n != (count + 3) >> 2; ++ n) {
#if __AVX__
    unsigned i0 = 4 * n;
    unsigned i2 = 4 * n + 2;
    __m256 x0 = _mm256_load_ps (x [i0]);
    __m256 x2 = _mm256_load_ps (x [i2]);
    __m256 v0 = _mm256_load_ps (v [i0]);
    __m256 v2 = _mm256_load_ps (v [i2]);
    __m256 x10 = _mm256_add_ps (x0, v0);
    __m256 x12 = _mm256_add_ps (x2, v2);
    _mm256_stream_ps (x [i0], x10);
    _mm256_stream_ps (x [i2], x12);
#else
    unsigned i0 = 4 * n;
    unsigned i1 = 4 * n + 1;
    unsigned i2 = 4 * n + 2;
    unsigned i3 = 4 * n + 3;
    v4f x0 = load4f (x [i0]);
    v4f x1 = load4f (x [i1]);
    v4f x2 = load4f (x [i2]);
    v4f x3 = load4f (x [i3]);
    v4f v0 = load4f (v [i0]);
    v4f v1 = load4f (v [i1]);
    v4f v2 = load4f (v [i2]);
    v4f v3 = load4f (v [i3]);
    v4f x10 = x0 + v0;
    v4f x11 = x1 + v1;
    v4f x12 = x2 + v2;
    v4f x13 = x3 + v3;
    _mm_stream_ps (x [i0], x10);
    _mm_stream_ps (x [i1], x11);
    _mm_stream_ps (x [i2], x12);
    _mm_stream_ps (x [i3], x13);
#endif
  }
}

// Update angular position u for motion with constant angular velocity w over a unit time interval.
// Use a single step of a second order method to integrate the differential equation defined by "tangent".
void advance_angular (float (* RESTRICT u) [4], float (* RESTRICT w) [4], unsigned count)
{
  v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
  v4f twopi = { 0x1.921fb6P+002f, 0x1.921fb6P+002f, 0x1.921fb6P+002f, 0x1.921fb6P+002f, };
  v4f lim1 = { +0x1.3c0000P+003f, 0.0f, 0.0f, 0.0f, }; // a touch over pi^2 (~ +0x1.3bd3ccP+003f)
  for (unsigned n = 0; n != count; ++ n) {
    v4f u1 = bch2 (load4f (w [n]), load4f (u [n]));
    // If |u|^2 exceeds lim1, scale u in order to adjust its length by -2pi.
    v4f xsq = dot (u1, u1);
    if (_mm_comigt_ss (xsq, lim1)) {
      u1 *= one - twopi * rsqrt (xsq);
    }
    store4f (u [n], u1);
  }
}

// Compute OpenGL modelview matrices from linear and angular position vectors, x and u.
void compute (char * RESTRICT buffer, std::size_t stride, const float (* RESTRICT x) [4], const float (* RESTRICT u) [4], const unsigned * permutation, unsigned count)
{
  v4f iiii = { 1.0f, 1.0f, 1.0f, 1.0f, };
#if __SSE4_1__
#else
  const union { std::uint32_t u [4]; v4f f; } mask = { { 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, }, };
  v4f oooi = { 0.0f, 0.0f, 0.0f, 1.0f, };
#endif
  char * iter = buffer;
  for (unsigned n = 0; n != count; ++ n, iter += stride) {
    unsigned m = permutation [n];
    float (& f) [16] = * reinterpret_cast <float (*) [16]> (iter);
    v4f u0 = load4f (u [m]);           // u0 u1 u2 0
    v4f usq = u0 * u0;                 // u0^2 u1^2 u2^2 0
    v4f uha = _mm_hadd_ps (usq, usq);
    v4f xsq = _mm_hadd_ps (uha, uha);  // x^2 x^2 x^2 x^2 (x = length of u)
    v4f ab = fg (xsq);
    v4f a = _mm_moveldup_ps (ab);
    v4f b = _mm_movehdup_ps (ab);
    v4f skew = a * u0;
    v4f u1 = _mm_shuffle_ps (u0, u0, SHUFFLE (1, 2, 0, 3)); // u1 u2 u0 0
    v4f u2 = _mm_shuffle_ps (u0, u0, SHUFFLE (2, 0, 1, 3)); // u2 u0 u1 0
    v4f symm = b * (u1 * u2);
    v4f sub = symm - skew;                       // s0 s1 s2  0
    v4f add = symm + skew;                       // a0 a1 a2  0
    v4f phicos = iiii + b * (usq - xsq);         // d0 d1 d2 cos(x)
    v4f aslo = _mm_movelh_ps (add, sub);         // a0 a1 s0 s1
    v4f ashi = _mm_unpackhi_ps (add, sub);       // a2 s2  0  0
    v4f ashd = _mm_movelh_ps (ashi, phicos);     // a2 s2 d0 d1
    v4f xyz0 = load4f (x [m]);
#if __SSE4_1__
    v4f phi = _mm_blend_ps (phicos, add, 8);     // d0 d1 d2  0
    v4f xyz1 = _mm_blend_ps (xyz0, iiii, 8);     // x0 x1 x2  1
#else
    v4f phi = _mm_and_ps (mask.f, phicos);
    v4f xyz1 = _mm_or_ps (xyz0, oooi);
#endif
    store4f (& f [0], _mm_shuffle_ps (ashd, sub, SHUFFLE (2, 0, 1, 3)));  // d0 a2 s1 0
    store4f (& f [4], _mm_shuffle_ps (ashd, add, SHUFFLE (1, 3, 0, 3)));  // s2 d1 a0 0
    store4f (& f [8], _mm_shuffle_ps (aslo, phi, SHUFFLE (1, 2, 2, 3)));  // a1 s0 d2 0
    store4f (& f [12], xyz1);                                             // x0 x1 x2 1
  }
}

// bch (u, v) for small v (roughly, |v| <= pi/4).
v4f rotate (v4f u, v4f v)
{
  // The integrator is designed to compute bch (u, v) for small u,
  // but here we want bch (u, v) for small v.
  // Make use of the formula bch (u, v) = - bch (- v, - u).
  return - bch4 (- v, - u);
  // If u has grown too large, it will get reduced in advance_angular.
}

// Restricted range [-pi/2, pi/2].
// Argument x x * *, result sin(x) cos(x) sin(x) cos(x).
v4f sincos (const v4f x)
{
  v4f oioi = { 0.0f, 1.0f, 0.0f, 1.0f, };
  v4f sc = sincos_internal (-x);   // -sin(x) 1-cos(x) -sin(x) 1-cos(x)
  return oioi - sc;                // sin(x) cos(x) sin(x) cos(x)
}

// Very restricted range [+0x1.8c97f0P-001f, +0x1.fb5486P-001f] ([0.774596691, 0.990879238]).
// Argument x x x x, result acos(x) acos(x) acos(x) acos(x).
v4f arccos (v4f x)
{
  return sqrt_nonzero (polyeval (x, apoly, apoly));
}
