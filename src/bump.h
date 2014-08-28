// -*- C++ -*-

#ifndef bump_h
#define bump_h

#include "vector.h"

// step_t: function object to compute the smoothstep function.

//  r ^          r = step (t)
//    |
//  1 +--------------XXXXXX--
//    |           X  |
//    |         X    |
//    |        X     |
//    |      X       |
//  0 +XXXX----------+------->
//        t0        t1        t

struct step_t
{
  v4f operator () (float t) const; // Returns { r, r, r, r }, where r = step (t);
  void initialize (float t0, float t1);
private:
  float c [4], T [4];
};

// bumps_t: function object to compute two independent "bump" functions.

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
  float t0, t1, t2, t3, v0, v1;
};

struct bumps_t
{
  v4f operator () (float t) const;
  void initialize (bump_specifier_t b0, bump_specifier_t b1);
private:
  float c [4] [4], S0 [4], T0 [4], U0 [4], V0 [4];
};

#endif
