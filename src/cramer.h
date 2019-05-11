// -*- C++ -*-

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

#ifndef cramer_h
#define cramer_h

// Interface.

namespace cramer
{
  // Invert the 3*3 matrix m, storing the inverse in minv.
  void inverse (const float (& m) [3] [4], float (& minv) [3] [4]);
}

// Implementation.

#include "vector.h"

void cramer::inverse (const float (& m) [3] [4], float (& minv) [3] [4])
{
  // Rows of m.
  v4f m0 = load4f (m [0]); // m00 m01 m02 0
  v4f m1 = load4f (m [1]); // m10 m11 m12 0
  v4f m2 = load4f (m [2]); // m20 m21 m22 0

  // Shuffled rows of m.
  v4f m0s = SHUFPS (m0, m0, (1, 2, 0, 3)); // m01 m02 m00 0
  v4f m1s = SHUFPS (m1, m1, (1, 2, 0, 3)); // m11 m12 m10 0
  v4f m2s = SHUFPS (m2, m2, (1, 2, 0, 3)); // m21 m22 m20 0

  // Shuffled cofactors.
  v4f c0s = m1 * m2s - m2 * m1s; // c02 c00 c01 0
  v4f c1s = m2 * m0s - m0 * m2s; // c12 c10 c11 0
  v4f c2s = m0 * m1s - m1 * m0s; // c22 c20 c21 0

  // Row 0 of the cofactor matrix.
  v4f c0r = SHUFPS (c0s, c0s, (1, 2, 0, 3)); // c00 c01 c02 0

  // Reciprocal determinant of m.
  v4f rdet = rcp (dot (m0, c0r));

  // Reshuffled cofactors.
  v4f c0t = _mm_unpacklo_ps (c0s, c1s); // c02 c12 c00 c10
  v4f c1t = _mm_unpackhi_ps (c0s, c1s); // c01 c11 0   0

  // Inverse of m (transposed cofactor matrix divided by determinant).
  store4f (minv [0], rdet * SHUFPS (c0t, c2s, (2, 3, 1, 3)));
  store4f (minv [1], rdet * SHUFPS (c1t, c2s, (0, 1, 2, 3)));
  store4f (minv [2], rdet * SHUFPS (c0t, c2s, (0, 1, 0, 3)));
}

#endif
