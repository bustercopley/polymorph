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
// Unspecified result if |a| >= 32767 * |b - a|.
float get_float (rng_t & rng, float a, float b)
{
  if (a < b || a > b) { // Ordered and not equal.
    // Get 48 random bits, do integer addition, convert to float, then scale.
    return ((std::int64_t) (rng.get () & 0x0000ffffffffffffull)
      + (std::int64_t) (0x1.0P+48f * (a / (b - a))))
    * ((b - a) * 0x1.0P-48f);
  }
  else { // Equal or unordered.
    return a;
  }
}

// Return a random vector uniformly distributed in a ball centre the origin.
v4f get_vector_in_ball (rng_t & rng, float radius)
{
  union {
    __m128i i128;
    std::uint64_t u64 [2];
    std::uint32_t u32 [4];
  };
  v4f v, vsq;
  do {
    u64 [0] = rng.get ();
    u64 [1] = rng.get ();
    u32 [3] = 0;
    v = _mm_cvtepi32_ps (i128);
    vsq = dot (v, v);
  }
  while (_mm_comige_ss (vsq, _mm_set_ss (0x1.0P+62f)));
  return _mm_set1_ps (0x1.000000P-31f * radius) * v;
}

// Return four random floats uniformly distributed in (-1,1)^4.
v4f get_vector_in_box (rng_t & rng)
{
  union {
    std::uint64_t u64 [2];
    __m128i i128;
  };
  u64 [0] = rng.get ();
  u64 [1] = rng.get ();
  v4f v = _mm_cvtepi32_ps (i128);
  return 0x1.000000P-031f * v;
}
