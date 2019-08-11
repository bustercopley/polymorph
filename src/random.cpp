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

#include "mswin.h"

#include "random.h"
#include "random-util.h"
#include "compiler.h"

// The xorshift family of PRNGs was introduced in [1]. This particular
// generator, "xorshift64* A_1(12; 25; 27).M_32", is suggested in [2].

// [1] George Marsaglia (2003)
// http://www.jstatsoft.org/article/view/v008i14/xorshift.pdf

// [2] Sebastiano Vigna (2014)
// http://arxiv.org/abs/1402.6246

void rng_t::initialize (std::uint64_t seed)
{
  state = 4101842887655102017ull;
  state ^= seed;
  state = get ();
}

NOINLINE
std::uint64_t rng_t::get ()
{
  state ^= state >> 12;
  state ^= state << 25;
  state ^= state >> 27;
  return state * 2685821657736338717ull;
}

// Random floating-point number uniformly distributed on the interval [a, b).
// Undefined behaviour due to signed integer overflow if |a| >= 32767 * |b - a|.
float get_float (rng_t & rng, float a, float b)
{
  if (a < b || a > b) { // Ordered and not equal.
    // Get 48 random bits, do integer addition, convert to float, then scale.
    float scale = (b - a) * 0x1.0P-48f;
    std::int64_t offset = a / scale;
    std::int64_t rand48 = rng.get () & 0xffffffffffffull;
    return scale * (rand48 + offset);
  }
  else { // Equal or unordered.
    return a;
  }
}

// Return a random vector uniformly distributed in a ball centre the origin.
v4f get_vector_in_ball (rng_t & rng, float radius)
{
  v4f v, vsq;
  do {
    auto a = rng.get ();
    auto b = rng.get () & 0x00000000ffffffffull; // Asume little-endian.
    v = _mm_cvtepi32_ps (_mm_set_epi64x (b, a));
    vsq = dot (v, v);
  }
  while (_mm_comige_ss (vsq, _mm_set_ss (0x1.0P+62f)));
  return _mm_set1_ps (0x1.000000P-31f * radius) * v;
}

// Return four random floats uniformly distributed in [-1,1)^4.
v4f get_vector_in_box (rng_t & rng)
{
  v4f v = _mm_cvtepi32_ps (_mm_set_epi64x (rng.get (), rng.get ()));
  return _mm_set1_ps (0x1.000000P-031f) * v;
}
