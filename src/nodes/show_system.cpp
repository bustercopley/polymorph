#include "show_system.h"
#include "snub_variance.h"
#include <vector>
#include <string>
#include <ostream>
#include <iomanip>
#include <iterator>
#include <algorithm>

// Helper functions for 'show_system'.

namespace
{
  // 'show_vectors' writes out co-ordinates for a third of
  // the vectors of a rotation system.

  inline std::ostream &
  show_vectors (std::ostream & stream,
                unsigned Np, const float (* x) [3], char name)
  {
    stream << std::fixed;
    stream.precision (16);

    stream << name << " [" << Np << "] = {\n";

    for (unsigned n = 0; n != Np; ++ n) {
      stream << "  {"
             << std::setw (20) << x [n] [0] << ", "
             << std::setw (20) << x [n] [1] << ", "
             << std::setw (20) << x [n] [2] << ", },\n";
    }

    return stream << "};\n";
  }

  inline std::ostream &
  show_array (std::ostream & stream,
              unsigned N, const uint8_t * P, char name)
  {
    stream << name << ':';
    for (unsigned n = 0; n != N; ++ n) {
      stream << ' ' << int (P [n]);
    }
    return stream << '\n';
  }
}

// 'show_system' produces a human-readable representation of the
// information in a 'system_t' object on the output stream 'stream'.

std::ostream &
show_system (std::ostream & stream,
             unsigned N, unsigned p, unsigned q, unsigned r,
             const uint8_t * P, const uint8_t * Q, const uint8_t * R,
             const uint8_t * X, const uint8_t * Y, const uint8_t * Z,
             const float (* x) [3], const float (* y) [3], const float (* z) [3],
             const float (& g) [8] [3])
{
  stream << "Rotation system <" << p << ", " << q << ", " << r << ">.\n";

  // Write the three permutations in disjoint cycle notation.
  stream << "\nPermutations of triangles:\n";
  show_array (stream, N, P, 'P');
  show_array (stream, N, Q, 'Q');
  show_array (stream, N, R, 'R');

  // Write the three nodes in each triangle.
  stream << "\nX-, Y- and Z-nodes of triangles:\n";
  show_array (stream, N, X, 'X');
  show_array (stream, N, Y, 'Y');
  show_array (stream, N, Z, 'Z');

  // The nodes, in rectangular cartesian co-ordinates.
  stream << "\nVectors.\n";
  show_vectors (stream, N / p, x, 'x');
  show_vectors (stream, N / q, y, 'y');
  show_vectors (stream, N / r, z, 'z');

  // Let a point P in a Moebius triangle be replicated in each of the
  // tiles of the Moebius triangulation. For certain points P, the set
  // of points so generated is the set of vertices of a uniform
  // polyhedron. There are eight such points, namely the three
  // vertices, three midpoints of edges, the incentre and another
  // point whose images are the vertices of a snub polyhedron.

  // The k are co-ordinates for the eight points of interest over the
  // (non-rectangular) basis {OA, OB, OC}, where O is the centre of
  // the sphere and A, B and C are the corners of the Moebius triangle.

  stream << "\nPoints.\n";
  for (unsigned n = 0; n != 8; ++ n) {
    stream << std::setw (2) << n << ": { "
           << std::setw (20) << g [n] [0] << ", "
           << std::setw (20) << g [n] [1] << ", "
           << std::setw (20) << g [n] [2] << " },\n";
  }

  if (p == 2 && q == 3) {
    double V = double (snub_variance (P, Q, R, X, Y, Z, x, y, z, g [7], N));
    return stream << "The snubs are equilateral up to variance "
                  << std::scientific << std::setprecision (16) << V << ".\n";
  }
  else {
    return stream << "Wrong configuration - can't check that the snubs are equilateral.\n";
  }
}
