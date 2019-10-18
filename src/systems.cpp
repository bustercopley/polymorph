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
#include <utility>

const unsigned symbols [] [3] = {
  { 2, 3, 3 }, // tetrahedral
  { 4, 3, 2 }, // octahedral
  { 2, 5, 3 }, // icosahedral
};

const float betas [] = {
  1.61803399f, // (1 + sqrt5)/2
  1.68501832f, // sqrt((4 + curt(19 - 3sqrt33) + curt(19 + 3sqrt33))/3)
  1.71556150f, // curt((9(1 + sqrt5) + sqrt(6(17 + 27sqrt5))) / 36) +
               // curt((9(1 + sqrt5) - sqrt(6(17 + 27sqrt5))) / 36)
};

void get_triangle (unsigned (& p) [3], bool reflect, float (& xyz) [3] [4],
                   float (& abc) [8] [4])
{
  // S [0] = sinA cosA sinA cosA
  // S [1] = sinB cosB sinB cosB
  // S [2] = sinC cosC sinC cosC
  v4f S [3];
  for (unsigned i = 0; i != 3; ++ i) {
    S [i] = sincos (_mm_set1_ps (3.14159265f / p [i]));
  }

  v4f t0 = _mm_unpacklo_ps (S [0], S [1]);               // sinA sinB cosA cosB
  v4f t1 = _mm_unpacklo_ps (S [2], _mm_setzero_ps ());   // sinC    0 cosC    0
  v4f vsinA = _mm_movelh_ps (t0, t1);                    // sinA sinB sinC 0
  v4f vcosA = _mm_movehl_ps (t1, t0);                    // cosA cosB cosC 0

  v4f vsinB = SHUFPS (vsinA, vsinA, (1, 2, 0, 3));       // sinB sinC sinA 0
  v4f vsinC = SHUFPS (vsinA, vsinA, (2, 0, 1, 3));       // sinC sinA sinB 0
  v4f vcosB = SHUFPS (vcosA, vcosA, (1, 2, 0, 3));       // cosB cosC cosA 0
  v4f vcosC = SHUFPS (vcosA, vcosA, (2, 0, 1, 3));       // cosC cosA cosB 0

  // Use the spherical cosine rule and the trigonometrical identity.
  v4f vcosa = (vcosA + vcosB * vcosC) / (vsinB * vsinC);   // cosa cosb cosc 0
  v4f vsina = sqrt (_mm_set1_ps (1.0) - vcosa * vcosa);  // sina sinb sinc 0

  ALIGNED16 float cosA [4], sinA [4], cosa [4], sina [4];
  store4f (cosA, vcosA);
  store4f (sinA, vsinA);
  store4f (cosa, vcosa);
  store4f (sina, vsina);

  cosa [3] = 0.0f;
  sina [3] = 0.0f;

  // xyz [0] =    1          0          0
  // xyz [1] = cosc       sinc          0
  // xyz [2] = cosb  cosA*sinb  sinA*sinb
  xyz [0] [0] = 1.0f;
  xyz [1] [0] = cosa [2];
  xyz [1] [1] = sina [2];
  xyz [2] [0] = cosa [1];
  xyz [2] [1] = cosA [0] * sina [1];
  xyz [2] [2] = sinA [0] * sina [1];

  // abc [0] =    1     0     0  (vertex X)
  // abc [1] =    0     1     0  (vertex Y)
  // abc [2] =    0     0     1  (vertex Z)
  // abc [3] =    0  sinb  sinc  (intersection of bisector of X with edge YZ)
  // abc [4] = sina     0  sinc  (intersection of bisector of Y with edge ZX)
  // abc [5] = sina  sinb     0  (intersection of bisector of Z with edge XY)
  // abc [6] = sinA  sinB  sinC  (incentre)
  // abc [7] = it's complicated  (snub generator)
  for (unsigned i = 0; i != 3; ++ i) {
    abc [i] [i] = 1.0f;
    store4f (abc [3 + i], vsina);
    abc [3 + i] [i] = 0.0f;
  }
  store4f (abc [6], vsinA);

  // Permutation pi where p [pi [i]] are in ascending order (indirect 3-sort).
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
    std::swap (pi [1], pi [2]);
  }

  // Coefficients abc [7] for the snubified polyhedron. beta0 is the positive
  // solution of the cubic equation x^3 - 2*x - 2*cosC = 0.
  float beta0 = betas [p [pi [2]] - 3];
  abc [7] [pi [0]] = 1.0f;
  abc [7] [pi [1]] = _mm_cvtss_f32 (S [pi [1]]) * beta0;
  abc [7] [pi [2]] = _mm_cvtss_f32 (S [pi [2]]) * (beta0 * beta0 - 1);

  if (reflect) {
    // Replace the triangle with a mirror image and adjust the coefficients.
    v4f z = load4f (xyz [2]);
    store4f (xyz [2], load4f (xyz [1]));
    store4f (xyz [1], z);
    xyz [1] [2] = -xyz [1] [2];
    for (unsigned k = 0; k != 8; ++ k) {
      v4f a = load4f (abc [k]);
      store4f (abc [k], SHUFPS (a, a, (0, 2, 1, 3)));
    }
    std::swap (p [1], p [2]);
  }

  // Normalize the coefficients.
  for (unsigned n = 3; n != 8; ++ n) {
    v4f T = _mm_setzero_ps ();
    for (unsigned i = 0; i != 3; ++ i) {
      T = T + _mm_set1_ps (abc [n] [i]) * load4f (xyz [i]);
    }
    store4f (abc [n], rsqrt (dot (T, T)) * load4f (abc [n]));
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
    get_triangle (p, n & 1, xyz [n], abc [n]);
    cramer::inverse (xyz [n], xyzinv [n]);
    unsigned N = make_system (p, xyz [n], nodes, indices);
    vao_ids [n] = make_vao (N, nodes, indices);
    primitive_count [n] = N;
  }
}
