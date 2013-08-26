// -*- C++ -*-
#ifndef hsv_to_rgb_h
#define hsv_to_rgb_h

#include "vector.h"

namespace
{
  inline v4f hsv_to_rgb (float hue, float saturation, float value, float alpha)
  {
    const v4f num1 = { 1.0f, 1.0f, 1.0f, 1.0f, };
    const v4f num2 = { 2.0f, 2.0f, 2.0f, 2.0f, };
    const v4f num3 = { 3.0f, 3.0f, 3.0f, 3.0f, };
    const v4f num5 = { 5.0f, 5.0f, 5.0f, 5.0f, };
    const v4f num6 = { 6.0f, 6.0f, 6.0f, 6.0f, };

    // Use offsets (1,5,3,*) to get [red, yellow, green, cyan, blue, magenta, red).
    const v4f offsets = { 1.0f, 5.0f, 3.0f, 0.0f, };
    v4f theta = _mm_set1_ps (hue) + offsets;

    // Reduce each component of theta modulo 6.0f.
    theta -= _mm_and_ps (_mm_cmpge_ps (theta, num6), num6);

    // Apply a function to each component of theta:

    // f[i] ^
    //      |
    //    1 +XXXXXXXX---+---+---+---X--
    //      |        X             X
    //      |         X           X
    //      |          X         X
    //    0 +---+---+---XXXXXXXXX---+--> theta[i]
    //      0   1   2   3   4   5   6

    // f [i] = 1 if theta [i] < 2,
    // f [i] = 3 - theta [i] if 2 <= theta [i] < 3,
    // f [i] = theta [i] - 5 if 5 <= theta [i],
    // f [i] = 0 otherwise.

    // The value of f [3] is unused.

    v4f lt2 = _mm_cmplt_ps (theta, num2);
    v4f lt3 = _mm_cmplt_ps (theta, num3);
    v4f m23 = _mm_andnot_ps (lt2, lt3);
    v4f ge5 = _mm_cmpge_ps (theta, num5);

    v4f term1 = _mm_and_ps (lt2, num1);
    v4f term2 = _mm_and_ps (m23, num3 - theta);
    v4f term3 = _mm_and_ps (ge5, theta - num5);

    v4f f = _mm_or_ps (_mm_or_ps (term1, term2), term3);

    v4f va = { value, value, value, alpha, };
    v4f s0 = { saturation, saturation, saturation, 0.0f, };
    v4f chroma = va * s0;     // x x x 0
    v4f base = va - chroma;   // y y y a
    return base + chroma * f; // r g b a
  }
}

#endif
