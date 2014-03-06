// -*- C++ -*-

#ifndef hsv_to_rgb_h
#define hsv_to_rgb_h

#include "vector.h"

namespace
{
  inline v4f hue_vector (float hue)
  {
    const v4f num1 = { 1.0f, 1.0f, 1.0f, 1.0f, };
    const v4f num2 = { 2.0f, 2.0f, 2.0f, 2.0f, };
    const v4f num3 = { 3.0f, 3.0f, 3.0f, 3.0f, };
    const v4f num5 = { 5.0f, 5.0f, 5.0f, 5.0f, };
    const v4f num6 = { 6.0f, 6.0f, 6.0f, 6.0f, };

    // The components of theta are [(hue + 1) % 6, (hue + 5) % 6, (hue + 3) % 6].
    // Offsets 1, 5, 3 give the sequence [red, yellow, green, cyan, blue, magenta, red).
    const v4f offsets = { 1.0f, 5.0f, 3.0f, 0.0f, };
    v4f theta = _mm_set1_ps (hue) + offsets;
    theta -= _mm_and_ps (_mm_cmpge_ps (theta, num6), num6);

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

    // f (x) = ((x<2) AND 1) OR (((x<2) XOR (x<3)) AND (3-x)) OR (((x>=5)) AND (x-5)).

    v4f lt2 = _mm_cmplt_ps (theta, num2);
    v4f lt3 = _mm_cmplt_ps (theta, num3);
    v4f b23 = _mm_xor_ps (lt2, lt3);
    v4f ge5 = _mm_cmpge_ps (theta, num5);

    v4f term1 = _mm_and_ps (lt2, num1);
    v4f term2 = _mm_and_ps (b23, num3 - theta);
    v4f term3 = _mm_and_ps (ge5, theta - num5);

    return _mm_or_ps (_mm_or_ps (term1, term2), term3);
  }

  // Assumes saturation and value have all four components equal, and alpha's first three components are zero.
  inline v4f hsv_to_rgb (float hue, v4f saturation, v4f value, v4f alpha)
  {
    v4f chroma = saturation * value;
    v4f base = value - chroma;
    v4f rgbx = base + chroma * hue_vector (hue);
#if __SSE4_1__
    return _mm_blend_ps (rgbx, alpha, 8); // rgba
#else
    const union { std::uint32_t u [4]; v4f f; } mask = { { 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, }, };
    return _mm_or_ps (_mm_and_ps (mask.f, rgbx), alpha);
#endif
  }
}

#endif
