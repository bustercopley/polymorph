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

#include "systems.h"
#include "compiler.h"
#include "cramer.h"
#include "graphics.h"
#include "make_system.h"
#include "resources.h"
#include "vector.h"
#include <cstdint>
#include <cstring>

struct triangle_t
{
  float xyz [3] [4]; // Coordinates for single triangle XYZ of Moebius tiling.
  float abc [8] [4]; // Coefficients A, B, C for generator T = AX + BY + CZ.
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

// { 1, 0, 0 },             Point X.
// { 0, 1, 0 },             Point Y.
// { 0, 0, 1 },             Point Z.
// { 0, sinb, sinc },       Intersection of YZ with bisector of angle X.
// { sina, 0, sinc },       Intersection of ZX with bisector of angle Y.
// { sina, sinb, 0 },       Intersection of YZ with bisector of angle Z.
// { sinA, sinB, sinC },    Incentre (intersection of the three bisectors).
// { alpha, beta, gamma },  Snub generator.

//     X1
//    /  \    Part of the tiling of the sphere, showing
//   Y0--Z0   triangles X1-Y0-Z0, X0-Y1-Z0, X0-Y0-Z1.
//   |\  /|
//   | X0 |
//   |/  \|
//   Z1  Y1

// For the snub generator, the three points
//   TX = alpha * X1 + beta * Y0 + gamma * Z0
//   TY = alpha * X0 + beta * Y1 + gamma * Z0
//   TZ = alpha * X0 + beta * Y0 + gamma * Z1
// (see diagram) are the vertices of an equilateral spherical triangle.

const ALIGNED16 triangle_t triangles [3] = {
  // Tetrahedral, <2, 3, 3>.
  {
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f },
      { +0x1.279a74P-1f, +0x1.a20bd8P-1f, +0x0.000000P+0f, 0.0f },
      { +0x1.279a74P-1f, +0x0.000000P+0f, +0x1.a20bd8P-1f, 0.0f },
    },
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f }, // 2
      { +0x0.000000P+0f, +0x1.000000P+0f, +0x0.000000P+0f, 0.0f }, // 1
      { +0x0.000000P+0f, +0x0.000000P+0f, +0x1.000000P+0f, 0.0f }, // 1
      { +0x0.000000P+0f, +0x1.3988e2P-1f, +0x1.3988e2P-1f, 0.0f }, // 11
      { +0x1.34bf64P-1f, +0x0.000000P+0f, +0x1.0b621eP-1f, 0.0f }, // 6
      { +0x1.34bf64P-1f, +0x1.0b621eP-1f, +0x0.000000P+0f, 0.0f }, // 6
      { +0x1.c9f25cP-2f, +0x1.8c97f0P-2f, +0x1.8c97f0P-2f, 0.0f }, // 7
      { +0x1.4cb7c0P-2f, +0x1.d2393eP-2f, +0x1.d2393eP-2f, 0.0f }, // 4
      // Great icosahedron:
      // { -0x1.605a90P+0f, +0x1.792eceP-1f, +0x1.792eceP-1f, 0.0f },
    },
  },
  // Octahedral, <2, 4, 3>.
  // Oriented so that two octahedral triangles cover one tetrahedral triangle.
  {
    { { +0x1.6a09e6P-1f, +0x1.000000P-1f, +0x1.000000P-1f, 0.0f },
      { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f },
      { +0x1.279a74P-1f, +0x1.a20bd8P-1f, +0x0.000000P+0f, 0.0f },
    },
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f }, // 11
      { +0x0.000000P+0f, +0x1.000000P+0f, +0x0.000000P+0f, 0.0f }, // 2
      { +0x0.000000P+0f, +0x0.000000P+0f, +0x1.000000P+0f, 0.0f }, // 3
      { +0x0.000000P+0f, +0x1.02ca46P-1f, +0x1.3cf3aeP-1f, 0.0f }, // 13
      { +0x1.1fd4a6P-1f, +0x0.000000P+0f, +0x1.f28990P-2f, 0.0f }, // 8
      { +0x1.43d136P-1f, +0x1.c9f25cP-2f, +0x0.000000P+0f, 0.0f }, // 7
      { +0x1.b9d594P-2f, +0x1.386c8eP-2f, +0x1.7ea3c6P-2f, 0.0f }, // 15
      { +0x1.31816eP-2f, +0x1.8d5502P-2f, +0x1.bdd092P-2f, 0.0f }, // 17
    },
  },
  // Icosahedral, <2, 5, 3>.
  {
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f },
      { +0x1.b38880P-1f, +0x1.0d2ca0P-1f, +0x0.000000P+0f, 0.0f },
      { +0x1.de4bd6P-1f, +0x0.000000P+0f, +0x1.6d62c6P-2f, 0.0f },
    },
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f }, // 12
      { +0x0.000000P+0f, +0x1.000000P+0f, +0x0.000000P+0f, 0.0f }, // 4
      { +0x0.000000P+0f, +0x0.000000P+0f, +0x1.000000P+0f, 0.0f }, // 5
      { +0x0.000000P+0f, +0x1.b4242aP-2f, +0x1.414c80P-1f, 0.0f }, // 14
      { +0x1.16fc4eP-1f, +0x0.000000P+0f, +0x1.e33798P-2f, 0.0f }, // 10
      { +0x1.4e5014P-1f, +0x1.890220P-2f, +0x0.000000P+0f, 0.0f }, // 9
      { +0x1.b3be36P-2f, +0x1.001f92P-2f, +0x1.795d50P-2f, 0.0f }, // 16
      { +0x1.287f1eP-2f, +0x1.52a52eP-2f, +0x1.b882c6P-2f, 0.0f }, // 18
    },
  },
};

// Turn a PQR triangle into a PRQ triangle.
void reflect (float (& abc) [8] [4],
              float (& xyz) [3] [4])
{
  v4f x = load4f (xyz [0]);
  v4f y = load4f (xyz [1]);
  v4f z = load4f (xyz [2]);

  v4f n = cross (x, y);                  // Perpendicular to x and y.
  v4f d = (dot (n, z) / dot (n, n)) * n; // Component of z parallel to n.

  store4f (xyz [1], z - (d + d));
  store4f (xyz [2], y);

  for (unsigned k = 0; k != 8; ++ k) {
    v4f a = load4f (abc [k]);
    store4f (abc [k], SHUFPS (a, a, (0, 2, 1, 3)));
  }
}

void initialize_systems (float (& abc) [system_count] [8] [4],
  float (& xyz) [system_count] [3] [4], float (& xyzinv) [system_count] [3] [4],
  unsigned (& primitive_count) [system_count],
  unsigned (& vao_ids) [system_count])
{
  ALIGNED16 float nodes [62] [4];
  std::uint8_t indices [60] [6];

  auto init = [&](const triangle_t & t, system_select_t select, unsigned q,
    unsigned r, bool reflected) -> void {

    std::memcpy (xyz [select], t.xyz, sizeof t.xyz);
    std::memcpy (abc [select], t.abc, sizeof t.abc);
    if (reflected) {
      reflect(abc [select], xyz [select]);
    }
    cramer::inverse (xyz [select], xyzinv [select]);
    unsigned p = 2, N = 2 * p * q * r / (q * r + r * p + p * q - p * q * r);
    make_system (q, r, xyz [select], nodes, indices);
    vao_ids [select] = make_vao (N, nodes, indices);
    primitive_count [select] = N;
  };

  for (unsigned n = 0; n != 3; ++ n) {
    init (triangles [n], system_select_t (2 * n + 0), 3 + n, 3, false);
    init (triangles [n], system_select_t (2 * n + 1), 3, 3 + n, true);
  }
}
