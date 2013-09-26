// -*- C++ -*-
#ifndef cramer_h
#define cramer_h

// Interface.

namespace cramer
{
  // Invert the 3 * 3 matrix m, storing the transpose in minvt.
  template <typename T, unsigned M, unsigned N>
  void inverse_transpose (const T (& m) [M] [N], float (& minvt) [3] [4]);
}

// Implementation.

#include "vector.h"

template <typename T, unsigned M, unsigned N>
void cramer::inverse_transpose (const T (& m) [M] [N], float (& minvt) [3] [4])
{
  // Rows of m.
  v4f m0 = load4f (m [0]); // m00 m01 m02 0
  v4f m1 = load4f (m [1]); // m10 m11 m12 0
  v4f m2 = load4f (m [2]); // m20 m21 m22 0

  v4f m0s = _mm_shuffle_ps (m0, m0, SHUFFLE (1, 2, 0, 3)); // m01 m02 m00 0
  v4f m1s = _mm_shuffle_ps (m1, m1, SHUFFLE (1, 2, 0, 3)); // m11 m12 m10 0
  v4f m2s = _mm_shuffle_ps (m2, m2, SHUFFLE (1, 2, 0, 3)); // m21 m22 m20 0

  v4f c0s = m1 * m2s - m2 * m1s; // m10m21-m20m11 m11m22-m21m12 m12m20-m22m10 0
  v4f c1s = m2 * m0s - m0 * m2s; // m20m01-m00m21 m21m02-m01m22 m22m00-m02m20 0
  v4f c2s = m0 * m1s - m1 * m0s; // m00m11-m10m01 m01m12-m11m02 m02m10-m12m00 0

  // Rows of c, the cofactor matrix.
  v4f c0 = _mm_shuffle_ps (c0s, c0s, SHUFFLE (1, 2, 0, 3)); // m11m22-m21m12 m12m20-m22m10 m10m21-m20m11 0
  v4f c1 = _mm_shuffle_ps (c1s, c1s, SHUFFLE (1, 2, 0, 3)); // m21m02-m01m22 m22m00-m02m20 m20m01-m00m21 0
  v4f c2 = _mm_shuffle_ps (c2s, c2s, SHUFFLE (1, 2, 0, 3)); // m01m12-m11m02 m02m10-m12m00 m00m11-m10m01 0

  v4f determinant = dot (m0, c0);
  v4f r = rcp (determinant);

  store4f (minvt [0], r * c0);
  store4f (minvt [1], r * c1);
  store4f (minvt [2], r * c2);
}

#endif
