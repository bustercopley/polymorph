// -*- C++ -*-

#ifndef bump_h
#define bump_h

//  r ^                        r = bump (t)
//    |
// v1 +--------------XXXXXX----------------
//    |           X  |    |  X
//    |         X    |    |    X
//    |        X     |    |     X
//    |      X       |    |       X
// v0 +XXXX----------+----+----------XXXX-->
//        t0        t1    t2        t3      t

#include "vector.h"
#include <limits>

struct step_t
{
  float c [4], T [4];
  void initialize (float start, float finish)
  {
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
    T [2] = 0.0f;
    T [3] = 0.0f;
  }

  float operator () (float t)
  {
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
};

struct bumps_t
{
  float S0 [4], T0 [4], U0 [4], V0 [4], c [4] [4];

  void initialize (float v00, float v01, float t00, float t01, float t02, float t03,
                   float v10, float v11, float t10, float t11, float t12, float t13)
  {
    v4f S = { t00, t02, t10, t12, };
    v4f T = { t01, t03, t11, t13, };
    v4f U = { v00, 0.0f, v10, 0.0f, };
    v4f V = { v01, v00 - v01, v11, v10 - v11 };
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
  v4f operator () (float t)
  {
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
    return _mm_hadd_ps (val, val);
  }
};

#endif
