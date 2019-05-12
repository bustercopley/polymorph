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

#ifndef rotor_h
#define rotor_h

#include "rodrigues.h"

struct rotor_t
{
  rotor_t (const float (& u) [4], float A);
  void operator () (const float (& in) [4], float (& out) [4]) const {
    store4f (out, mapply (matrix, load4f (in)));
  }
private:
  float matrix [4] [4];
};

inline rotor_t::rotor_t (const float (& u) [4], float a)
{
  ALIGNED16 float X [4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  ALIGNED16 float U [4];
  store4f (U, _mm_set1_ps (a) * _mm_load_ps (u));
  const unsigned permutation [1] = { 0 };
  compute ((char *) & matrix, 0, & X, & U, permutation, 1);
}

#endif
