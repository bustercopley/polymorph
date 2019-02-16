// -*- C++ -*-

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
  // Returns { f, f, f, f } where f = step(t);
  v4f operator () (float t) const;
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
  // Returns {f, g, f, g} where f = bump0(t), g = bump1(t).
  v4f operator () (float t) const;
  void initialize (const bump_specifier_t & b0, const bump_specifier_t & b1);
private:
  float c [4] [4], S0 [4], T0 [4], U0 [4], V0 [4];
};

#endif
