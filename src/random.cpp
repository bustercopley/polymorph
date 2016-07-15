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

  // 18480	  12580	    224	  31284	   7a34	.obj/x64/tiny/polymorph.exe
  // 19264	  11464	    104	  30832	   7870	.obj/x86/tiny/polymorph.exe

  // 18464	  12580	    224	  31268	   7a24	.obj/x64/tiny/polymorph.exe
  // 19248	  11464	    104	  30816	   7860	.obj/x86/tiny/polymorph.exe

  // 18448	  12580	    224	  31252	   7a14	.obj/x64/tiny/polymorph.exe
  // 19236	  11464	    104	  30804	   7854	.obj/x86/tiny/polymorph.exe

// Random floating-point number uniformly distributed on the interval [a, b).
float get_float (rng_t & rng, float a, float b)
{
  return a + 0x1.000000P-064f * (b - a) * rng.get ();
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
  v4f lim = { 0x1.000000P+062f, 0.0f, 0.0f, 0.0f, };
  do {
    u64 [0] = rng.get ();
    u64 [1] = rng.get () & 0xffffffff;
    v = _mm_cvtepi32_ps (i128);
    vsq = dot (v, v);
  }
  while (_mm_comige_ss (vsq, lim));
  return _mm_set1_ps (0x1.000000P-31f * radius) * v;
}

// Return a random vector uniformly distributed in
// the box [0,1)^3.
v4f get_vector_in_box (rng_t & rng)
{
  union {
    __m128i i128;
    std::uint64_t u64 [2];
  };
  u64 [0] = rng.get () & 0x7fffffff7fffffff;
  u64 [1] = rng.get () & 0x7fffffff;
  v4f v = _mm_cvtepi32_ps (i128);
  return _mm_set1_ps (0x1.000000P-031f) * v;
}
