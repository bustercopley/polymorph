// -*- C++ -*-

// Copyright 2016 Richard Copley
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

#ifndef random_util_h
#define random_util_h

#include "random.h"
#include "vector.h"

float get_float (rng_t & rng, float a, float b);
v4f get_vector_in_box (rng_t & rng);
v4f get_vector_in_ball (rng_t & rng, float radius);

#endif
