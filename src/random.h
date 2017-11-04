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

#ifndef random_h
#define random_h

#include <cstdint>

struct rng_t
{
  rng_t () = default;
  void initialize (std::uint64_t seed);
  std::uint64_t get ();
private:
  std::uint64_t state;
  rng_t (const rng_t &) = delete;
  rng_t & operator = (const rng_t &) = delete;
};

#endif
