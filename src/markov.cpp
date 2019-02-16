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
#include "markov.h"
#include "random.h"
#include "rodrigues.h"
#include "compiler.h"

// The procedure below gives rise to roughly the following relative polyhedron abundancies.

// 8.10 % tetrahedron
// 8.66 % octahedron
// 4.68 % cube
// 7.70 % icosahedron
// 4.62 % dodecahedron
// 5.81 % truncated tetrahedron
// 9.55 % truncated octahedron
// 3.48 % truncated cube
// 3.46 % truncated icosahedron
// 3.47 % truncated dodecahedron
// 7.78 % cuboctahedron
// 4.62 % icosidodecahedron
// 3.53 % rhombicuboctahedron
// 3.46 % rhombicosidodecahedron
// 7.23 % rhombitruncated cuboctahedron
// 6.93 % rhombitruncated icosidodecahedron
// 3.44 % snub cube
// 3.46 % snub dodecahedron

inline bool operator == (const polyhedron_select_t & x, const polyhedron_select_t & y)
{
  return x.system == y.system && x.point == y.point;
}

const unsigned probability_max = 1u << 31u;
const unsigned probability_mask = probability_max - 1u;
const struct replacement_t
{
  polyhedron_select_t before, after;
  unsigned probability;
} replacements [] = {
  // The fixups in maybe_perform_replacement assume this ordering.
  { { tetrahedral, 0, }, { octahedral,  1, }, unsigned (0.375f * probability_max), },
  { { tetrahedral, 6, }, { octahedral,  5, }, unsigned (0.375f * probability_max), },
  { { tetrahedral, 3, }, { octahedral,  0, }, unsigned (0.375f * probability_max), },
  { { octahedral,  1, }, { tetrahedral, 0, }, unsigned (0.375f * probability_max), },
  { { octahedral,  5, }, { tetrahedral, 6, }, unsigned (0.375f * probability_max), },
  { { octahedral,  0, }, { tetrahedral, 3, }, unsigned (0.375f * probability_max), },
  { { icosahedral, 1, }, { tetrahedral, 7, }, unsigned (0.400f * probability_max), },
  { { tetrahedral, 7, }, { icosahedral, 1, }, unsigned (0.600f * probability_max), },
  // The snub tetrahedron is not chiral (it is the icosahedron).
  // Not doing this replacement makes some snub-desnub combos impossible.
  { { tetrahedral, 7, }, { dual_tetrahedral, 7, }, probability_max / 2, },
};
const unsigned replacement_count = sizeof replacements / sizeof * replacements;

// The fundamental triangles of the tetrahedral and octahedral tilings
// are chosen so that no rotation is needed for replacements 0 to 5.
// Rotations 0, 1, 2 correspond to replacements 6, 7, 8 respectively.
ALIGNED16 const float rotations [3] [4] = {
  // Rotate about a p-node through angle pi/4.
  { -0x1.921fb6P-001f, 0.0f, 0.0f, 0.0f, }, // I1 -> T7
  { +0x1.921fb6P-001f, 0.0f, 0.0f, 0.0f, }, // T7 -> I1
  // Rotate about an r-node through angle approximately 0.2471 pi.
  { +0x1.caf0fcP-002f, +0x1.448542P-001f, 0.0f, 0.0f, }, // T7 -> T7*
};

inline bool bernoulli_trial (rng_t & rng, unsigned probability)
{
  return (rng.get () & probability_mask) < probability;
}

inline void maybe_perform_replacement (rng_t & rng, float (& u) [4], polyhedron_select_t & current, unsigned & starting_point)
{
  // Replacements are in terms of the primary representation so mask out the dual bit for now.
  unsigned duality = current.system & 1;
  current.system = system_select_t (current.system & ~1);
  // If non-snub, maybe switch between dual representations, to permit either
  // variety of any snub or desnub operation that follows. Not doing it now,
  // before the replacements, makes some double-desnub combos impossible.
  unsigned entropy = (unsigned) rng.get (); // Seems a shame to waste it.
  duality ^= entropy & (current.point != 7);

  unsigned m = 0;
  while (m != replacement_count && ! (replacements [m].before == current && bernoulli_trial (rng, replacements [m].probability))) ++ m;
  if (m != replacement_count) {
    current = replacements [m].after;
    // Fixups after the replacement: avoid backtracking transitions
    // by updating starting_point, and maybe appply a rotation.
    if (m < 6) {
      // Replacements 0 - 5 (tetrahedral <-> octahedral): do replacement
      // on the starting point as well. For example, this prohibits
      // T0 (octahedron) -> T6 = O5 (truncated octahedron) -> O1(octahedron).
      // No rotation is needed for these replacements.
      unsigned base = m - m % 3;
      unsigned j = 0;
      while (j != 3 && starting_point != replacements [base + j].before.point) ++ j;
      starting_point = j == 3 ? current.point : replacements [base + j].after.point;
    }
    else {
      // Replacements 6 - 8: apply a rotation (adjust the object's orientation).
      v4f rotation = load4f (rotations [m - 6]);
      if (duality) rotation = - rotation;
      store4f (u, rotate (load4f (u), rotation));
      // 6, 7 (tetrahedral <-> icosahedral): no transition is forbidden after these replacements.
      // 8 (tetrahedral -> dual tetrahedral (both snub)): keep the starting_point we already have.
      if (m != 8) starting_point = current.point;
    }
  }

  // If non-snub, maybe switch between dual representations (again).
  // Not doing this now, after the replacement, makes some double-snub combos impossible.
  duality ^= (entropy >> 1) & (current.point != 7);
  // Restore the dual bit.
  current.system = system_select_t (current.system ^ duality);
}

void transition (rng_t & rng, float (& u) [4], polyhedron_select_t & current, unsigned & starting_point)
{
  maybe_perform_replacement (rng, u, current, starting_point);

  union
  {
    std::uint64_t data;
    char disallowed [8];
  };

  data = 0xf8c0bcbab927170f;

  // Perform a Markov transition.
  unsigned next;
  do next = rng.get () & 7; /* PLEASE */

  // Certain transitions are not allowed:
  while (next == starting_point || 1 & disallowed [current.point] >> next);

  starting_point = current.point;
  current.point = next;
}
