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

#ifndef model_h
#define model_h

#include "compiler.h"
#include "random.h"
#include "graphics.h"
#include "bump.h"
#include "markov.h"
#include "settings.h"
#include "object.h"
#include <cstdint>

struct model_t
{
  ~model_t ();

  bool initialize (std::uint64_t seed);
  bool start (int width, int height, const settings_t & settings);
  void draw_next ();
private:
  void nodraw_next ();
  void set_capacity (std::size_t new_capacity);
  void recalculate_locus (unsigned index);
  void draw (unsigned begin, unsigned count);
  void bounce (unsigned ix, unsigned iy);
  void wall_bounce (unsigned iw, unsigned iy);
  void kdtree_search ();

  void * memory;
  object_t * objects;
  unsigned * object_order;
  unsigned * kdtree_index;
  float * kdtree_split;

  float (* x) [4];  // position
  float (* v) [4];  // velocity
  float (* u) [4];  // angular position
  float (* w) [4];  // angular velocity
  float (* e) [4];  // locus end

  float radius;
  float animation_speed_constant;

  std::size_t capacity;
  std::size_t kdtree_capacity;
  unsigned count;
  unsigned primitive_count [system_count]; // = { 12, 24, 60, }
  std::uint32_t vao_ids [system_count];

  ALIGNED16 float walls [6] [2] [4];
  ALIGNED16 float abc [system_count] [8] [4];
  ALIGNED16 float xyz [system_count] [3] [4];
  ALIGNED16 float xyzinv [system_count] [3] [4];
  ALIGNED16 bumps_t bumps;
  ALIGNED16 step_t step;

  program_t program;
  rng_t rng;
};

#endif
