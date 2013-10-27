// -*- C++ -*-

#ifndef bump_h
#define bump_h

#include "vector.h"

// Function object to compute the smoothstep function:
//   f(t) = 0 for t < start,
//   f(t) = 1 for t > finish,
// and f is smoothly interpolated by a cubic polynomial in between.
struct step_t
{
  v4f operator () (float t) const;
  void initialize (float start, float finish);
  float c [4], T [4];
};

//  r ^                        r = bump (t)
//    |
// v1 +--------------XXXXXX----------------
//    |           X  |    |  X
//    |         X    |    |    X
//    |        X     |    |     X
//    |      X       |    |       X
// v0 +XXXX----------+----+----------XXXX-->
//        t0        t1    t2        t3      t

struct bump_specifier_t
{
  float v0, v1, t0, t1, t2, t3;
};

// Object computes two independent "bump" functions.
struct bumps_t
{
  void operator () (float t, float & v0, float & v1) const;
  void initialize (bump_specifier_t b0, bump_specifier_t b1);
private:
  float S0 [4], T0 [4], U0 [4], V0 [4], c [4] [4];
};

#endif
