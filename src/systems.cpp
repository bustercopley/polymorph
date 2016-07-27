#include "mswin.h"
#include "systems.h"
#include "compiler.h"
#include "resources.h"
#include "vector.h"
#include "make_system.h"
#include "cramer.h"
#include "graphics.h"
#include <cstdint>
#include <cstring>

struct triangle_t
{
  float xyz [3] [4]; // Coordinates for a single triangle XYZ of the Moebius tiling.
  float abc [8] [4]; // Coefficients A, B, C for the generator T = AX + BY + CZ.
};

// The fundamental triangle of the tiling has angles
// A = pi/p, B = pi/q, C = pi/r and sides a, b, c.

// By the spherical cosine rule,
// cosa = (cosA + cosB * cosC) / (sinB * sinC),
// cosb = (cosB + cosC * cosA) / (sinC * sinA),
// cosc = (cosC + cosA * cosB) / (sinA * sinB).

// We assume p = 2; thus cosA = 0 and sinA = 1. Therefore,
// cosa = (cosB * cosC) / (sinB * sinC),
// cosb = cosB / sinC,
// cosc = cosC / sinB.

// Any triple (alpha, beta, gamma) identifies a point T relative to
// the spherical triangle XYZ, by the formula
// T = alpha * X + beta * Y + gamma * Z.
// For certain triples, replicating the point T in each triangle
// of the tiling gives the vertices of a uniform polyhedron:

// { 1, 0, 0, },             Point X.
// { 0, 1, 0, },             Point Y.
// { 0, 0, 1, },             Point Z.
// { 0, sinb, sinc, },       Intersection of YZ with the angular bisector of corner X.
// { sina, 0, sinc, },       Intersection of ZX with the angular bisector of corner Y.
// { sina, sinb, 0, },       Intersection of YZ with the angular bisector of corner Z.
// { sinA, sinB, sinC, },    Incentre (intersection of the three bisectors).
// { alpha, beta, gamma, },  Snub generator.

//     X1
//    /  \    Part of the tiling of the sphere, showing
//   Z0--Y0   the triangle X0-Y0-Z0 and the adjacent triangles.
//   |\  /|
//   | X0 |
//   |/  \|
//   Y1  Z1

// For the snub generator, the three points
//   TX = alpha * X1 + beta * Y0 + gamma * Z0
//   TY = alpha * X0 + beta * Y1 + gamma * Z0
//   TZ = alpha * X0 + beta * Y0 + gamma * Z1
// (see diagram) are the vertices of an equilateral spherical triangle.

ALIGNED16 triangle_t triangles [3] =
{
  // Tetrahedral, <2, 3, 3>.
  {
    { { +0x1.000000P+000f, +0x0.000000P+000f, +0x0.000000P+000f, 0.0f, },
      { +0x1.279a74P-001f, +0x1.a20bd8P-001f, +0x0.000000P+000f, 0.0f, },
      { +0x1.279a74P-001f, +0x0.000000P+000f, +0x1.a20bd8P-001f, 0.0f, },
    },
    { { +0x1.000000P+000f, +0x0.000000P+000f, +0x0.000000P+000f, 0.0f, }, // Octahedron
      { +0x0.000000P+000f, +0x1.000000P+000f, +0x0.000000P+000f, 0.0f, }, // Tetrahedron
      { +0x0.000000P+000f, +0x0.000000P+000f, +0x1.000000P+000f, 0.0f, }, // Tetrahedron
      { +0x0.000000P+000f, +0x1.3988e2P-001f, +0x1.3988e2P-001f, 0.0f, }, // Cuboctahedron
      { +0x1.34bf64P-001f, +0x0.000000P+000f, +0x1.0b621eP-001f, 0.0f, }, // Truncated tetrahedron
      { +0x1.34bf64P-001f, +0x1.0b621eP-001f, +0x0.000000P+000f, 0.0f, }, // Truncated tetrahedron
      { +0x1.c9f25cP-002f, +0x1.8c97f0P-002f, +0x1.8c97f0P-002f, 0.0f, }, // Truncated octahedron
      { +0x1.4cb7c0P-002f, +0x1.d2393eP-002f, +0x1.d2393eP-002f, 0.0f, }, // Icosahedron
      // { -0x1.605a90P+000f, +0x1.792eceP-001f, +0x1.792eceP-001f, 0.0f, }, // Great icosahedron
    },
  },
  // Octahedral, <2, 4, 3>.
  // Oriented so that two octahedral triangles cover one tetrahedral triangle.
  {
    { { +0x1.6a09e6P-001f, +0x1.000000P-001f, +0x1.000000P-001f, 0.0f, },
      { +0x1.000000P+000f, +0x0.000000P+000f, +0x0.000000P+000f, 0.0f, },
      { +0x1.279a74P-001f, +0x1.a20bd8P-001f, +0x0.000000P+000f, 0.0f, },
    },
    { { +0x1.000000P+000f, +0x0.000000P+000f, +0x0.000000P+000f, 0.0f, }, // Cuboctahedron
      { +0x0.000000P+000f, +0x1.000000P+000f, +0x0.000000P+000f, 0.0f, }, // Octahedron
      { +0x0.000000P+000f, +0x0.000000P+000f, +0x1.000000P+000f, 0.0f, }, // Cube
      { +0x0.000000P+000f, +0x1.02ca46P-001f, +0x1.3cf3aeP-001f, 0.0f, }, // Rhombicuboctahedron
      { +0x1.1fd4a6P-001f, +0x0.000000P+000f, +0x1.f28990P-002f, 0.0f, }, // Truncated cube
      { +0x1.43d136P-001f, +0x1.c9f25cP-002f, +0x0.000000P+000f, 0.0f, }, // Truncated octahedron
      { +0x1.b9d594P-002f, +0x1.386c8eP-002f, +0x1.7ea3c6P-002f, 0.0f, }, // Rhombitruncated cuboctahedron
      { +0x1.31816eP-002f, +0x1.8d5502P-002f, +0x1.bdd092P-002f, 0.0f, }, // Snub cube
    },
  },
  // Icosahedral, <2, 5, 3>.
  {
    { { +0x1.000000P+000f, +0x0.000000P+000f, +0x0.000000P+000f, 0.0f, },
      { +0x1.b38880P-001f, +0x1.0d2ca0P-001f, +0x0.000000P+000f, 0.0f, },
      { +0x1.de4bd6P-001f, +0x0.000000P+000f, +0x1.6d62c6P-002f, 0.0f, },
    },
    { { +0x1.000000P+000f, +0x0.000000P+000f, +0x0.000000P+000f, 0.0f, }, // Icosidodecahedron
      { +0x0.000000P+000f, +0x1.000000P+000f, +0x0.000000P+000f, 0.0f, }, // Icosahedron
      { +0x0.000000P+000f, +0x0.000000P+000f, +0x1.000000P+000f, 0.0f, }, // Dodecahedron
      { +0x0.000000P+000f, +0x1.b4242aP-002f, +0x1.414c80P-001f, 0.0f, }, // Rhombicosidodecahedron
      { +0x1.16fc4eP-001f, +0x0.000000P+000f, +0x1.e33798P-002f, 0.0f, }, // Truncated dodecahedron
      { +0x1.4e5014P-001f, +0x1.890220P-002f, +0x0.000000P+000f, 0.0f, }, // Truncated icosahedron
      { +0x1.b3be36P-002f, +0x1.001f92P-002f, +0x1.795d50P-002f, 0.0f, }, // Rhombitruncated icosidodecahedron
      { +0x1.287f1eP-002f, +0x1.52a52eP-002f, +0x1.b882c6P-002f, 0.0f, }, // Snub dodecahedron
    },
  },
};

// Turn a PQR triangle into a PRQ triangle.
void reflect (triangle_t & t)
{
  v4f x = load4f (t.xyz [0]);
  v4f y = load4f (t.xyz [1]);
  v4f z = load4f (t.xyz [2]);

  v4f n = cross (x, y);
  v4f d = (dot (n, z) / dot (n, n)) * n; // Component of z perpendicular to x and y.

  store4f (t.xyz [1], z - (d + d));
  store4f (t.xyz [2], y);

  for (unsigned k = 0; k != 8; ++ k) {
    v4f a = load4f (t.abc [k]);
    store4f (t.abc [k], _mm_shuffle_ps (a, a, SHUFFLE (0, 2, 1, 3)));
  }
}

void initialize_systems (float (& abc) [system_count] [8] [4],
                         float (& xyz) [system_count] [3] [4],
                         float (& xyzinvt) [system_count] [3] [4],
                         unsigned (& primitive_count) [system_count],
                         unsigned (& vao_ids) [system_count])
{
  ALIGNED16 float nodes [62] [4];
  std::uint8_t indices [60] [6];

  auto init = [&] (const triangle_t & t, system_select_t select, unsigned q, unsigned r) -> void
  {
    std::memcpy (xyz [select], t.xyz, sizeof t.xyz);
    std::memcpy (abc [select], t.abc, sizeof t.abc);
    cramer::inverse_transpose (xyz [select], xyzinvt [select]);
    unsigned p = 2, N = 2 * p * q * r / (q * r + r * p + p * q - p * q * r);
    make_system (q, r, t.xyz, nodes, indices);
    vao_ids [select] = make_vao (N, nodes, indices);
    primitive_count [select] = N;
  };

  for (unsigned n = 0; n != 3; ++ n) {
    triangle_t & triangle = triangles [n];
    init (triangle, system_select_t (2 * n + 0), 3 + n, 3);
    reflect (triangle);
    init (triangle, system_select_t (2 * n + 1), 3, 3 + n);
  }
}
