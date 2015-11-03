#include "mswin.h"
#include "random.h"
#include "compiler.h"

// The xorshift family of PRNGs was introduced in [1].
// This particular generator, "xorshift64* A_1(12; 25; 27).M_32", is suggested in [2].

// [1] George Marsaglia (2003) http://www.jstatsoft.org/article/view/v008i14/xorshift.pdf
// [2] Sebastiano Vigna (2014) http://arxiv.org/abs/1402.6246

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

#include "random-util.h"

// Random floating-point number uniformly distributed on the interval [a, b).
float get_float (rng_t & rng, float a, float b)
{
  return a + 0x1.0p-64f * rng.get () * (b - a);
}

// Return a random vector uniformly distributed in
// the interior of a sphere centre the origin.
v4f get_vector_in_ball (rng_t & rng, float radius)
{
  union {
    __m128i i128;
    std::uint64_t u64 [2];
  };
  v4f v, vsq;
  v4f lim = { 0x1.0p62f, 0.0f, 0.0f, 0.0f, };
  do {
    u64 [0] = rng.get ();
    u64 [1] = rng.get () & 0xffffffffull;
    v = _mm_cvtepi32_ps (i128);
    vsq = dot (v, v);
  }
  while (_mm_comige_ss (vsq, lim));
  float rs = 0x1.0p-31f * radius;
  v4f k = { rs, rs, rs, 0.0f, };
  return k * v;
}

// Return a random vector uniformly distributed in
// the box [0,1)^3.
v4f get_vector_in_box (rng_t & rng)
{
  union {
    __m128i i128;
    std::uint64_t u64 [2];
  };
  u64 [0] = rng.get () & 0x7fffffff7fffffffull;
  u64 [1] = rng.get () & 0x7fffffffull;
  v4f v = _mm_cvtepi32_ps (i128);
  v4f k = { 0x1.0p-31f, 0x1.0p-31f, 0x1.0p-31f, 0.0f };
  return k * v;
}
