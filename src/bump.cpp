#include "bump.h"
#include "vector.h"
#include <limits>

void step_t::initialize (float start, float finish)
{
  // Precompute the four coefficents of the cubic polynomial for the smoothstep region.
  v4f temp = {
    start * start * (3.0f * finish - start),
    -6.0f * start * finish,
    +3.0f * (start + finish),
    -2.0f,
  };
  v4f dt = _mm_set1_ps (finish - start);
  store4f (c, temp / (dt * dt * dt));
  T [0] = start;
  T [1] = finish;
  T [2] = 0.0f; // unused
  T [3] = 0.0f; // unused
}

float step_t::operator () (float t) const
{
  // Evaluate the polynomial by Estrin's method.
  // Mask in 0, 1 or the value according to the region to which t belongs.
  static const float inf = std::numeric_limits <float>::infinity ();
  v4f c4 = load4f (c);
  v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
  v4f ttt = { t, t, t, t, };
  v4f tt = _mm_unpacklo_ps (one, ttt); // 1 t 1 t
  v4f f0 = c4 * tt;                    // c0 c1*t c2 c3*t
  v4f ha = _mm_hadd_ps (f0, f0) * tt * tt;
  v4f f = _mm_hadd_ps (ha, ha);        // f f f f
  v4f f1 = _mm_unpacklo_ps (f, one);   // f 1 * *
  v4f lo = load4f (T);
  v4f hi = { T [1], +inf, -inf, -inf, };
  v4f select = _mm_andnot_ps (_mm_cmplt_ps (ttt, lo), _mm_cmplt_ps (ttt, hi));
  v4f values = _mm_and_ps (select, f1);
  return _mm_cvtss_f32 (_mm_hadd_ps (values, values));
}

void bumps_t::initialize (bump_specifier_t b0, bump_specifier_t b1)
{
  // Precompute the coefficients of four cubic polynomials in t, giving
  // the two smoothstep regions of the each of the two bump functions.
  v4f S = { b0.t0, b0.t2, b1.t0, b1.t2, };
  v4f T = { b0.t1, b0.t3, b1.t1, b1.t3, };
  v4f U = { b0.v0, 0.0f, b1.v0, 0.0f, };
  v4f V = { b0.v1, b0.v0 - b0.v1, b1.v1, b1.v0 - b1.v1 };
  v4f ntwo = { -2.0f, -2.0f, -2.0f, -2.0f, };
  v4f three = { 3.0f, 3.0f, 3.0f, 3.0f, };
  v4f dt = T - S;
  v4f m = (V - U) / (dt * dt * dt);
  store4f (c [3], ntwo * m);
  store4f (c [2], three * m * (S + T));
  store4f (c [1], ntwo * three * m * S * T);
  store4f (c [0], U + m * S * S * (three * T - S));
  store4f (S0, S);
  store4f (T0, T);
  store4f (U0, U);
  store4f (V0, V);
}

// Returns {f,g,f,g}, where f = bump0 (t), g = bump1 (t).
void bumps_t::operator () (float t, float & v0, float & v1) const
{
  // Compute all four polynomials by Estrin's method, and mask
  // and combine the values according to the region of the graph
  // to which t belongs.
  v4f s = { t, t, t, t, };
  v4f S = load4f (S0);
  v4f T = load4f (T0);
  v4f U = load4f (U0);
  v4f V = load4f (V0);
  v4f f01 = load4f (c [0]) + load4f (c [1]) * s;
  v4f f12 = load4f (c [2]) + load4f (c [3]) * s;
  v4f f = f01 + f12 * s * s;
  v4f ltS = _mm_cmplt_ps (s, S);
  v4f ltT = _mm_cmplt_ps (s, T);
  v4f val_ltS = _mm_and_ps (ltS, U);
  v4f val_mST = _mm_and_ps (_mm_andnot_ps (ltS, ltT), f);
  v4f val_geT = _mm_andnot_ps (ltT, V);
  v4f val = _mm_or_ps (_mm_or_ps (val_ltS, val_geT), val_mST);
  float fgfg [4];
  store4f (fgfg, _mm_hadd_ps (val, val));
  v0 = fgfg [0];
  v1 = fgfg [1];
}
