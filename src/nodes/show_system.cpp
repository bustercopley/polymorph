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
                unsigned Np, const float (* x) [4], char name)
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
             const float (* xyz) [4],
             const float (& abc) [8] [4],
             const std::uint8_t (* indices) [6])
{
  stream << "Rotation system <" << p << ", " << q << ", " << r << ">; N = " << N << ".\n";

  // TODO: dump indices.

  // The nodes, in rectangular cartesian co-ordinates.
  stream << "\nVectors.\n";
  show_vectors (stream, N + 2, xyz, 'X');

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
           << std::setw (20) << abc [n] [0] << ", "
           << std::setw (20) << abc [n] [1] << ", "
           << std::setw (20) << abc [n] [2] << " },\n";
  }

  auto Var = snub_variance (xyz, indices, abc [7], N);
  return stream << "The snub triangles are of mean side "
                << std::setprecision (4)
                << std::fixed << std::acos (Var.first)
                << " radians and are equilateral up to a variance of "
                << std::scientific << std::setprecision (4)
                << Var.second << ".\n";
}
