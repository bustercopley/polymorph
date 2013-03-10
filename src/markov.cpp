#include "markov.h"
#include "random.h"
#include "rodrigues.h"

// The procedure below gives rise to roughly the following relative polyhedron abundancies.

// 12.0 % tetrahedron
// 11.5 % octahedron
//  5.7 % cube
//  7.7 % icosahedron
//  5.1 % dodecahedron
//  6.7 % truncated tetrahedron
//  8.2 % truncated octahedron
//  3.2 % truncated cube
//  2.9 % truncated icosahedron
//  2.9 % truncated dodecahedron
//  8.9 % cuboctahedron
//  5.1 % icosidodecahedron
//  3.2 % rhombicuboctahedron
//  2.9 % rhombicosidodecahedron
//  4.9 % rhombitruncated cuboctahedron
//  4.4 % rhombitruncated icosidodecahedron
//  2.3 % snub cube
//  2.2 % snub dodecahedron

struct replacement_t
{
  polyhedron_select_t before, after;
  float rotation [3];
  float probability;
};

const float pi = 3.141592654f;

static const replacement_t replacements [] =
{
  { { tetrahedral, 0, 0, }, { octahedral,  2, 0, }, { 0, 0, + pi / 2, }, 48, },
  { { tetrahedral, 6, 0, }, { octahedral,  4, 0, }, { 0, 0, + pi / 2, }, 48, },
  { { tetrahedral, 3, 0, }, { octahedral,  0, 0, }, { 0, 0, + pi / 2, }, 48, },
  { { octahedral,  2, 0, }, { tetrahedral, 0, 0, }, { 0, 0, + pi / 2, }, 48, },
  { { octahedral,  4, 0, }, { tetrahedral, 6, 0, }, { 0, 0, + pi / 2, }, 48, },
  { { octahedral,  0, 0, }, { tetrahedral, 3, 0, }, { 0, 0, + pi / 2, }, 48, },
  { { icosahedral, 2, 0, }, { tetrahedral, 7, 1, }, { + pi / 4, 0, 0, }, 21, },
  { { icosahedral, 2, 0, }, { tetrahedral, 7, 2, }, { - pi / 4, 0, 0, }, 25, },
  { { tetrahedral, 7, 1, }, { icosahedral, 2, 0, }, { - pi / 4, 0, 0, }, 84, },
  { { tetrahedral, 7, 2, }, { icosahedral, 2, 0, }, { + pi / 4, 0, 0, }, 84, },
  { { tetrahedral, 7, 1, }, { tetrahedral, 7, 2, }, { + pi / 2, 0, 0, }, 64, },
  { { tetrahedral, 7, 2, }, { tetrahedral, 7, 1, }, { + pi / 2, 0, 0, }, 64, },
  // { { tetrahedral, 1, 0, }, { tetrahedral, 2, 0, }, { + pi / 2, 0, 0, }, 64, },
  // { { tetrahedral, 2, 0, }, { tetrahedral, 1, 0, }, { + pi / 2, 0, 0, }, 64, },
  // { { tetrahedral, 4, 0, }, { tetrahedral, 5, 0, }, { + pi / 2, 0, 0, }, 64, },
  // { { tetrahedral, 5, 0, }, { tetrahedral, 4, 0, }, { + pi / 2, 0, 0, }, 64, },
};

static const unsigned replacement_count = sizeof replacements / sizeof * replacements;

inline bool operator == (const polyhedron_select_t & x, const polyhedron_select_t & y)
{
  return x.system == y.system && x.point == y.point && x.program == y.program;
}

void transition (rng_t & rng, float (& u) [4], polyhedron_select_t & current, unsigned & starting_point)
{
  if (current.point != 7) {
    current.program = 0;
  }

  for (unsigned m = 0; m != replacement_count; ++ m) {
    const replacement_t f = replacements [m];
    if (current == f.before && (rng.get () & 127) < f.probability) {
      current = f.after;
      rotate (u, f.rotation);
      if (f.before.system == tetrahedral && current.system == octahedral) {
        if (starting_point == 0) starting_point = 2;
        else if (starting_point == 6) starting_point = 4;
        else if (starting_point == 3) starting_point = 0;
        else starting_point = current.point;
      }
      else if (f.before.system == octahedral && current.system == tetrahedral) {
        if (starting_point == 2) starting_point = 0;
        else if (starting_point == 4) starting_point = 6;
        else if (starting_point == 0) starting_point = 3;
        else starting_point = current.point;
      }
      else starting_point = current.point;
      break;
    }
  }

  unsigned next;
  do next = rng.get () & 7; /* PLEASE */

  // Certain transitions are not allowed:
  while (next == starting_point || 1 & current.point ["\001\002\004\270\270\270\300\370"] >> next);

  starting_point = current.point;
  current.point = next;

  // Choose the shader program.
  if (current.program == 0 && current.point == 7) {
    current.program = 1 + (rng.get () & 1);
  }
}
