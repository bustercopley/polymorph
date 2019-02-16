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
  v4f m0s = _mm_shuffle_ps (m0, m0, SHUFFLE (1, 2, 0, 3)); // m01 m02 m00 0
  v4f m1s = _mm_shuffle_ps (m1, m1, SHUFFLE (1, 2, 0, 3)); // m11 m12 m10 0
  v4f m2s = _mm_shuffle_ps (m2, m2, SHUFFLE (1, 2, 0, 3)); // m21 m22 m20 0

  // Shuffled cofactors.
  v4f c0s = m1 * m2s - m2 * m1s; // m10m21-m20m11 m11m22-m21m12 m12m20-m22m10 0 = c02 c00 c01 0
  v4f c1s = m2 * m0s - m0 * m2s; // m20m01-m00m21 m21m02-m01m22 m22m00-m02m20 0 = c12 c10 c11 0
  v4f c2s = m0 * m1s - m1 * m0s; // m00m11-m10m01 m01m12-m11m02 m02m10-m12m00 0 = c22 c20 c21 0

  // Row 0 of the cofactor matrix.
  v4f c0r = _mm_shuffle_ps (c0s, c0s, SHUFFLE (1, 2, 0, 3)); // c00 c01 c02 0

  // Reciprocal determinant of m.
  v4f rdet = rcp (dot (m0, c0r));

  // Reshuffled cofactors.
  v4f c0t = _mm_unpacklo_ps (c0s, c1s); // c02 c12 c00 c10
  v4f c1t = _mm_unpackhi_ps (c0s, c1s); // c01 c11 0   0

  // Inverse of m (transposed cofactor matrix divided by determinant).
  store4f (minv [0], rdet * _mm_shuffle_ps (c0t, c2s, SHUFFLE (2, 3, 1, 3))); // rdet * (c00 c10 c20 0)
  store4f (minv [1], rdet * _mm_shuffle_ps (c1t, c2s, SHUFFLE (0, 1, 2, 3))); // rdet * (c01 c11 c21 0)
  store4f (minv [2], rdet * _mm_shuffle_ps (c0t, c2s, SHUFFLE (0, 1, 0, 3))); // rdet * (c02 c12 c22 0)
}

#endif
