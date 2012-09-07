#include "show_system.h"
#include "snub_variance.h"
#include <vector>
#include <string>
#include <ostream>
#include <iomanip>
#include <iterator>
#include <algorithm>

// Helper functions for 'show_system'.

namespace {
  // 'show_vectors' writes out co-ordinates for a third of
  // the vectors of a rotation system.

  inline std::ostream &
  show_vectors (std::ostream & stream,
                unsigned Np, const float (* x) [3], char name) {
    stream << std::fixed;
    stream.precision (12);

    stream << name << " [" << Np << "] = {\n";

    for (unsigned n = 0; n != Np; ++ n) {
      stream << "  {" <<
        std::setw (15) << x [n] [0] << ", " <<
        std::setw (15) << x [n] [1] << ", " <<
        std::setw (15) << x [n] [2] << ", },\n";
    }

    return stream << "};\n";
  }

  // 'putat' puts the decimal representation of 'n' into 's'
  // in such a way that the units digit is in column 'pos'.

  inline void
  putat (std::string & s, unsigned pos, unsigned n) {
    do {
      s [pos] = '0' + char (n % 10);
      -- pos;
      n /= 10;
    }
    while (n);
  }

  // 'put_aspect' formats a third of the combinatorial map
  // data from a rotation system at a column.

  inline void
  put_aspect (std::vector <std::string> & lines, unsigned column,
              unsigned Np, unsigned p, const unsigned char * x) {
    for (unsigned k = 0; k != Np; ++ k) {
      lines [k * p + p - 1].replace (column + 1, 13, "_____________", 13);
      putat (lines [k * p], column + 4, k);

      for (unsigned d = 0; d != 2 * p; ++ d) {
        putat (lines [k*p + d/2], column + (d%2 ? 12 : 8), int (x [2*k*p + d]));
      }
    }
  }
}

// 'show_system' produces a human-readable representation of the
// information in a 'system_t' object on the output stream 'stream'.

std::ostream &
show_system (std::ostream & stream,
             unsigned N, unsigned p, unsigned q, unsigned r,
             const unsigned char * x, const unsigned char * y, const unsigned char * z,
             const unsigned char (* s) [4],
             const float (* u) [3], const float (* v) [3], const float (* w) [3],
             const float (& k) [8] [3]) {
  stream << std::fixed << std::setprecision (12);

  // Write the rotation system in tabular format.
  // This is the primary combinatorial information.

  stream << '<' << p << ' ' << q << ' ' << r << "> (N = " << N << ")\n" <<
    "+-------------+-------------+-------------+\n"
    "|   X   y   z |   Y   z   x |   Z   x   y |\n"
    "+-------------+-------------+-------------+\n";

  std::vector <std::string> lines (N);
  for (unsigned n = 0; n != N; ++ n) {
    lines [n] = "|             |             |             |\n";
  };

  put_aspect (lines, 0, N / p, p, x);
  put_aspect (lines, 14, N / q, q, y);
  put_aspect (lines, 28, N / r, r, z);

  std::copy (lines.begin (), lines.end (), std::ostream_iterator <std::string> (stream));

  // Secondary combinatorial data, derived from the
  // information in the first table.

  stream << "\nSecondary combinatorial data.\n";
  for (unsigned n = 0; n != N / p; ++ n) {
    stream << std::setw (2) << n << ":"
           << std::setw (3) << int(s [n] [0])
           << std::setw (3) << int(s [n] [1])
           << std::setw (3) << int(s [n] [2])
           << std::setw (3) << int(s [n] [3]) << "\n";
  }

  // The nodes, in rectangular cartesian co-ordinates.

  stream << "\nVectors.\n";
  show_vectors (stream, N / p, u, 'u');
  show_vectors (stream, N / q, v, 'v');
  show_vectors (stream, N / r, w, 'w');

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
           << std::setw (15) << k [n] [0] << ", "
           << std::setw (15) << k [n] [1] << ", "
           << std::setw (15) << k [n] [2] << " },\n";
  }

  if (p == 2 && q == 3) {
    long double V = snub_variance (u, v, w, k [7], N / 2, x, s);
    return stream << "The snubs are equilateral up to variance "
                  << std::scientific << std::setprecision (20) << V << ".\n";
  }
  else {
    return stream << "Wrong configuration - can't check that the snubs are equilateral.\n";
  }
}
