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
#include "rodrigues.h"
#include <cstdint>
#include <cstring>

const unsigned symbols [] [3] = {
  { 2, 3, 3 },
  { 4, 3, 2 },
  { 2, 5, 3 },
};

const float betas [] = {
  1.61803399f, // (1 + sqrt5)/2
  1.68501832f, // sqrt((4 + curt(19 - 3sqrt33) + curt(19 + 3sqrt33))/3)
  1.71556150f, // curt((9(1 + sqrt5) + sqrt(6(17 + 27sqrt5))) / 36) +
               // curt((9(1 + sqrt5) - sqrt(6(17 + 27sqrt5))) / 36)
};

struct triangle_t
{
  float xyz [3] [4]; // Three unit vectors, corners of a Moebius triangle XYZ.
  float abc [8] [4]; // Coefficients of uniform generators aX + bY + cZ.
};

triangle_t get_triangle (const unsigned (& p) [3]) {
  ALIGNED16 float S [3] [4], cosa [4], sina [4];
  for (int i = 0; i != 3; ++ i) {
    store4f (S [i], sincos (_mm_set1_ps (3.14159265f / p [i])));
  }

  // The spherical cosine rule.
  cosa [0] = (S [0] [1] + S [1] [1] * S [2] [1]) / (S [1] [0] * S [2] [0]);
  cosa [1] = (S [1] [1] + S [2] [1] * S [0] [1]) / (S [2] [0] * S [0] [0]);
  cosa [2] = (S [2] [1] + S [0] [1] * S [1] [1]) / (S [0] [0] * S [1] [0]);
  cosa [3] = 0.0f;

  // The trigonometrical identity.
  v4f sina_v4f = sqrt (_mm_set1_ps (1.0) - load4f (cosa) * load4f (cosa));
  store4f (sina, sina_v4f);

  ALIGNED16 triangle_t triangle = {
    {
      { 1, 0, 0, 0 },
      { cosa [2], sina [2], 0, 0 },
      { cosa [1], S [0] [1] * sina [1], S [0] [0] * sina [1], 0 },
    },
    {
      { 1, 0, 0, 0 }, // Vertex A
      { 0, 1, 0, 0 }, // Vertex B
      { 0, 0, 1, 0 }, // Vertex C
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 },
    },
  };

  store4f (triangle.abc [3], sina_v4f);
  store4f (triangle.abc [4], sina_v4f);
  store4f (triangle.abc [5], sina_v4f);

  triangle.abc [3] [0] = 0.0f;
  triangle.abc [4] [1] = 0.0f;
  triangle.abc [5] [2] = 0.0f;

  triangle.abc [6] [0] = S [0] [0];
  triangle.abc [6] [1] = S [1] [0];
  triangle.abc [6] [2] = S [2] [0];

  unsigned pi [3] = { 0, 1, 2 };

  if (p [0] > p [1]) {
    pi [0] = 1;
    pi [1] = 0;
  }

  if (p [pi [0]] > p [2]) {
    pi [2] = pi [0];
    pi [0] = 2;
  }

  if (p [pi [1]] > p [pi [2]]) {
    unsigned t = pi [1];
    pi [1] = pi [2];
    pi [2] = t;
  }

  float beta0 = betas [p [pi [2]] - 3];

  triangle.abc [7] [pi [0]] = 1.0f;
  triangle.abc [7] [pi [1]] = S [pi [1]] [0] * beta0;
  triangle.abc [7] [pi [2]] = S [pi [2]] [0] * (beta0 * beta0 - 1);

  for (int n = 0; n != 8; ++ n) {
    v4f T = _mm_setzero_ps ();
    for (int i = 0; i != 3; ++ i) {
      T = T + _mm_set1_ps (triangle.abc [n] [i]) * load4f (triangle.xyz [i]);
    }
    store4f (triangle.abc [n], rsqrt (dot (T, T)) * load4f (triangle.abc [n]));
  }

  return triangle;
}

// Turn a PQR triangle into a PRQ triangle.
void reflect (triangle_t & triangle)
{
  v4f x = load4f (triangle.xyz [0]);
  v4f y = load4f (triangle.xyz [1]);
  v4f z = load4f (triangle.xyz [2]);

  v4f n = cross (x, y);                  // Perpendicular to x and y.
  v4f d = (dot (n, z) / dot (n, n)) * n; // Component of z parallel to n.

  store4f (triangle.xyz [1], z - (d + d));
  store4f (triangle.xyz [2], y);

  for (unsigned k = 0; k != 8; ++ k) {
    v4f a = load4f (triangle.abc [k]);
    store4f (triangle.abc [k], SHUFPS (a, a, (0, 2, 1, 3)));
  }
}

void initialize_systems (float (& abc) [system_count] [8] [4],
  float (& xyz) [system_count] [3] [4], float (& xyzinv) [system_count] [3] [4],
  unsigned (& primitive_count) [system_count],
  unsigned (& vao_ids) [system_count])
{
  ALIGNED16 float nodes [62] [4];
  std::uint8_t indices [60] [6];

  for (unsigned n = 0; n != 6; ++ n) {
    unsigned p [3];
    std::memcpy (p, symbols [n / 2], sizeof p);
    ALIGNED16 triangle_t triangle = get_triangle (p);
    if (n & 1) {
      auto temp = p [1];
      p [1] = p [2];
      p [2] = temp;
      reflect (triangle);
    }
    std::memcpy (xyz [n], triangle.xyz, sizeof triangle.xyz);
    std::memcpy (abc [n], triangle.abc, sizeof triangle.abc);
    cramer::inverse (xyz [n], xyzinv [n]);
    unsigned N = make_system (p, xyz [n], nodes, indices);
    vao_ids [n] = make_vao (N, nodes, indices);
    primitive_count [n] = N;
  }
}
