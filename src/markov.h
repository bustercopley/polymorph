// -*- C++ -*-

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

#ifndef markov_h
#define markov_h

#include "systems.h"

struct rng_t;

struct polyhedron_select_t
{
  system_select_t system;
  unsigned point;
};

void transition (rng_t & rng, float (& u) [4], polyhedron_select_t & current, unsigned & starting_point);

#endif
