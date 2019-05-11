// Copyright 2012-2019 Richard Copley
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
#include "bump.h"
#include <limits>

void step_t::initialize (float t0, float t1)
{
  // Precompute the coefficents c of the cubic polynomial f
  // such that f(t0)=0, f(t1)=1, f'(t0)=0 and f'(t0)=1.
  float d = t1 - t0;
  float a = t1 + t0;
  c [0] = t0 * t0 * (a + d + d);
  c [1] = -6 * t0 * t1;
  c [2] = 3 * a;
  c [3] = -2;

  // Divide c [] by d^3.
  v4f dt = _mm_set1_ps (d);
  store4f (c, load4f (c) / (dt * dt * dt));

  T [0] = t0;
  T [1] = t1;
  T [2] = t1;
  T [3] = std::numeric_limits <float>::infinity ();
}

v4f step_t::operator () (float t) const
{
  // Evaluate the polynomial f by Estrin's method. Return
  //   (0 0 0 0)  if t < t0,
  //   (f f f f)  if t0 <= t < t1,
  //   (1 1 1 1)  if t > t1.
  v4f c4 = load4f (c);
  v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
  v4f tttt = _mm_set1_ps (t);           // t t t t
  v4f tt = _mm_unpacklo_ps (one, tttt); // 1 t 1 t
  v4f f0 = c4 * tt;                     // c0 c1*t c2 c3*t
  v4f ha = _mm_hadd_ps (f0, f0) * tt * tt;
  v4f f = _mm_hadd_ps (ha, ha);         // f f f f
  v4f f1 = _mm_unpacklo_ps (f, one);    // f 1 f 1
  v4f tx = load4f (T);                  // t0  t1 t1 inf
  v4f lo = _mm_movelh_ps (tx, tx);      // t0  t1 t0  t1
  v4f hi = _mm_movehl_ps (tx, tx);      // t1 inf t1 inf
  v4f sel = _mm_and_ps (_mm_cmpge_ps (tttt, lo), _mm_cmplt_ps (tttt, hi));
  v4f val = _mm_and_ps (sel, f1);       // f? 1? f? 1?
  return _mm_hadd_ps (val, val);
}

void bumps_t::initialize (
  const bump_specifier_t & b0, const bump_specifier_t & b1)
{
  // Precompute the coefficients of four cubic polynomials in t, giving
  // the two smoothstep regions of the each of the two bump functions.
  v4f b0t = load4f (& b0.t0); // b0.t0 b0.t1 b0.t2 b0.t2
  v4f b1t = load4f (& b1.t0); // b1.t0 b1.t1 b1.t2 b1.t2
  v4f b0v = _mm_movelh_ps (load4f (& b0.v0), _mm_setzero_ps ()); // b0.v0 b0.v1
  v4f b1v = _mm_movelh_ps (load4f (& b1.v0), _mm_setzero_ps ()); // b1.v0 b1.v1
  v4f S = SHUFPS (b0t, b1t, (0, 2, 0, 2)); // b0.t0 b0.t2 b1.t0 b1.t2
  v4f T = SHUFPS (b0t, b1t, (1, 3, 1, 3)); // b0.t1 b0.t3 b1.t1 b1.t3
  v4f U = SHUFPS (b0v, b1v, (0, 2, 0, 2)); // b0.v0   0   b1.v0   0
  v4f V1 = SHUFPS (b0v, b1v, (1, 0, 1, 0)); // b0.v1 b0.v0 b1.v1 b1.v0
  v4f V2 = SHUFPS (b0v, b1v, (2, 1, 2, 1)); //   0   b0.v1   0   b1.v1
  v4f V = V1 - V2;
  v4f d = T - S;
  v4f a = T + S;
  v4f m = (V - U) / (d * d * d);
  store4f (c [0], U + m * S * S * (a + d + d));
  store4f (c [1], _mm_set1_ps (-6.0f) * m * S * T);
  store4f (c [2], _mm_set1_ps (+3.0f) * m * a);
  store4f (c [3], _mm_set1_ps (-2.0f) * m);
  store4f (S0, S);
  store4f (T0, T);
  store4f (U0, U);
  store4f (V0, V);
}

// Returns {f,g,f,g}, where f = bump0 (t), g = bump1 (t).
v4f bumps_t::operator () (float t) const
{
  // Compute all four polynomials by Estrin's method, and mask and combine the
  // values according to the region of the graph to which t belongs.
  v4f s = _mm_set1_ps (t);
  v4f S = load4f (S0);
  v4f T = load4f (T0);
  v4f U = load4f (U0);
  v4f V = load4f (V0);
  v4f f01 = load4f (c [0]) + load4f (c [1]) * s;
  v4f f12 = load4f (c [2]) + load4f (c [3]) * s;
  v4f f = f01 + f12 * s * s;
  v4f ltS = _mm_cmplt_ps (s, S);
  v4f geT = _mm_cmpge_ps (s, T);
  v4f x1 = _mm_andnot_ps (_mm_or_ps (ltS, geT), f);
  v4f x2 = _mm_and_ps (ltS, U);
  v4f x3 = _mm_and_ps (geT, V);
  v4f val = _mm_or_ps (_mm_or_ps (x1, x2), x3);
  return _mm_hadd_ps (val, val);
}
