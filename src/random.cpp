#include "random.h"
#include "vector.h"
#include "compiler.h"

// The algorithm is D1(A1r), aka Ranq1, from Numerical Recipes.

void rng_t::initialize (std::uint64_t seed)
{
  state = 4101842887655102017ull;
  state ^= seed;
  state = get ();
}

NOINLINE
std::uint64_t rng_t::get ()
{
  state ^= state >> 21;
  state ^= state << 35;
  state ^= state >> 4;
  return state * 2685821657736338717ull;
}

// Random double number uniformly distributed on the interval [a, b).
double rng_t::get_double (double a, double b)
{
  return a + 0x1.0p-64 * get () * (b - a);
}

// Return a random vector uniformly distributed in
// the interior of a sphere centre the origin.
v4f rng_t::get_vector_in_ball (float radius)
{
  union {
    __m128i i128;
    std::uint64_t u64 [2];
  };
  v4f v, sq;
  v4f lim = { 0x1.0p62f, 0.0f, 0.0f, 0.0f, };
  do {
    u64 [0] = get ();
    u64 [1] = get () & 0xffffffffull;
    v = _mm_cvtepi32_ps (i128);
    sq = dot (v, v);
  }
  while (_mm_comige_ss (sq, lim));
  float rs = 0x1.0p-31 * radius;
  v4f k = { rs, rs, rs, 0.0f, };
  return k * v;
}

// Return a random vector uniformly distributed in
// the box [0,1)^3.
v4f rng_t::get_vector_in_box ()
{
  union {
    __m128i i128;
    std::uint64_t u64 [2];
  };
  u64 [0] = get () & 0x7fffffff7fffffffull;
  u64 [1] = get () & 0x7fffffffull;
  v4f v = _mm_cvtepi32_ps (i128);
  v4f k = { 0x1.0p-31f, 0x1.0p-31f, 0x1.0p-31f, 0.0f };
  return k * v;
}
