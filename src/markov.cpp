#include "markov.h"
#include "random.h"
#include "rodrigues.h"
#include "compiler.h"

// The procedure below gives rise to roughly the following relative polyhedron abundancies.

// 8.4 % tetrahedron
// 8.5 % octahedron
// 4.9 % cube
// 7.7 % icosahedron
// 4.5 % dodecahedron
// 6.1 % truncated tetrahedron
// 9.9 % truncated octahedron
// 3.1 % truncated cube
// 3.4 % truncated icosahedron
// 3.4 % truncated dodecahedron
// 7.8 % cuboctahedron
// 4.5 % icosidodecahedron
// 3.6 % rhombicuboctahedron
// 3.4 % rhombicosidodecahedron
// 7.2 % rhombitruncated cuboctahedron
// 6.7 % rhombitruncated icosidodecahedron
// 3.5 % snub cube
// 3.4 % snub dodecahedron

struct replacement_t
{
  polyhedron_select_t before, after;
  float rotation [3];
  unsigned probability;
};

const float pi = 0x1.921fb4P1f;

static const replacement_t replacements [] =
{
  { { tetrahedral, 0, }, { octahedral,  1, }, { 0, 0, 0, }, 48, },
  { { tetrahedral, 6, }, { octahedral,  5, }, { 0, 0, 0, }, 48, },
  { { tetrahedral, 3, }, { octahedral,  0, }, { 0, 0, 0, }, 48, },
  { { octahedral,  1, }, { tetrahedral, 0, }, { 0, 0, 0, }, 48, },
  { { octahedral,  5, }, { tetrahedral, 6, }, { 0, 0, 0, }, 48, },
  { { octahedral,  0, }, { tetrahedral, 3, }, { 0, 0, 0, }, 48, },
  { { icosahedral, 1, }, { tetrahedral, 7, }, { - pi / 4, 0, 0, }, 51, },
  { { tetrahedral, 7, }, { icosahedral, 1, }, { + pi / 4, 0, 0, }, 72, },
  { { tetrahedral, 7, }, { dual_tetrahedral, 7, }, { pi / 2, 0, 0, }, 64, }, // snub, but not chiral
};

static const unsigned replacement_count = sizeof replacements / sizeof * replacements;

inline bool operator == (const polyhedron_select_t & x, const polyhedron_select_t & y)
{
  return x.system == y.system && x.point == y.point;
}

void transition (rng_t & rng, float (& u) [4], polyhedron_select_t & current, unsigned & starting_point)
{
  // Transitions are in terms of the primary representation so mask out the dual bit for now.
  unsigned duality = unsigned (current.system) & 1;
  current.system = system_select_t (unsigned (current.system) & ~1);

  // If non-snub, maybe switch between dual representations. Doing this before the
  // replacements allows a snub dodecahedron to double-desnub in two different ways.
  if (current.point != 7) {
    duality ^= rng.get () & 1;
  }

  for (unsigned m = 0; m != replacement_count; ++ m) {
    const replacement_t f = replacements [m];
    if (current == f.before && (rng.get () & 127) < f.probability) {
      current = f.after;
      if (m >= 6)
      {
        float rot [4]  ALIGNED16 = { f.rotation [0], f.rotation [1], f.rotation [2], 0.0f, };
        if (duality) store4f (rot, - load4f (rot));
        rotate (u, & rot [0]);
      }
      if (f.before.system == tetrahedral && f.after.system == octahedral) {
        if (starting_point == 0) starting_point = 1;
        else if (starting_point == 6) starting_point = 5;
        else if (starting_point == 3) starting_point = 0;
        else starting_point = current.point;
      }
      else if (f.before.system == octahedral && f.after.system == tetrahedral) {
        if (starting_point == 1) starting_point = 0;
        else if (starting_point == 5) starting_point = 6;
        else if (starting_point == 0) starting_point = 3;
        else starting_point = current.point;
      }
      else if (f.before.system == tetrahedral && f.after.system == dual_tetrahedral) {
        // keep the starting point we had before.
      }
      else {
        starting_point = current.point;
      }
      break;
    }
  }

  // Perform a Markov transition.
  unsigned next;
  do next = rng.get () & 7; /* PLEASE */

  // Certain transitions are not allowed:
  while (next == starting_point || 1 & current.point ["\017\027\047\271\272\274\300\370"] >> next);

  starting_point = current.point;
  current.point = next;

  // Set the dual bit.
  current.system = system_select_t (current.system ^ duality);
}
