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

#ifndef hsv_to_rgb_h
#define hsv_to_rgb_h

#include "vector.h"

inline v4f hue_vector (float hue)
{
  // Offsets 1/6, 5/6, 3/6 give [red, yellow, green, cyan, blue, magenta, red).
  const v4f offsets = { 0x1.555556p-3f, 0x1.aaaaaap-1f, 0.5f, 0.0f };
  v4f temp = _mm_set1_ps (hue) + offsets;
  v4f theta = // fmod (temp, 6.0)
    _mm_set1_ps (6.0f) * (temp - _mm_cvtepi32_ps (_mm_cvttps_epi32 (temp)));

  // Apply the function f to each component of theta, where:
  //   f (x) = 1      if x < 2,
  //   f (x) = 3 - x  if 2 <= x < 3,
  //   f (x) = 0      if 3 <= x < 5,
  //   f (x) = x - 5  if 5 <= x.

  //    y ^
  //      |
  //    1 +XXXXXXXX---+---+---+---X  y = f (x)
  //      |        X             X
  //      |         X           X
  //      |          X         X
  //    0 +---+---+---XXXXXXXXX---+--> x
  //      0   1   2   3   4   5   6

  // f (x) = (x<2) ? 1 : (x<3) ? (3-x) : (x>=5) ? (x-5) : 0.

  v4f lt2 = theta < _mm_set1_ps (2.0f);
  v4f lt3 = theta < _mm_set1_ps (3.0f);
  v4f b23 = _mm_xor_ps (lt2, lt3);
  v4f ge5 = theta >= _mm_set1_ps (5.0f);

  v4f term1 = _mm_and_ps (lt2, _mm_set1_ps (1.0f));
  v4f term2 = _mm_and_ps (b23, _mm_set1_ps (3.0f) - theta);
  v4f term3 = _mm_and_ps (ge5, theta - _mm_set1_ps (5.0f));

  return _mm_or_ps (_mm_or_ps (term1, term2), term3);
}

// Assumes saturation and value have all four components equal, and alpha's
// first three components are zero.
inline v4f hsv_to_rgb (float hue, v4f saturation, v4f value, v4f alpha)
{
  v4f rgbx = value - (value * saturation) * hue_vector (hue);
#if __SSE4_1__
  return _mm_blend_ps (rgbx, alpha, 8); // rgba
#else
  v4f mask = _mm_castsi128_ps (_mm_setr_epi32 (-1, -1, -1, 0));
  return _mm_or_ps (_mm_and_ps (mask, rgbx), alpha);
#endif
}

#endif
