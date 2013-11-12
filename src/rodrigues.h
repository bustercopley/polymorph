// -*- C++ -*-

#ifndef rodrigues_h
#define rodrigues_h

#include "vector.h"
#include <cstdint>

// State: three-dimensional vectors x, u, v, w.
//   x: position (defined in point_t)
//   v: velocity
//   u: angular position
//   w: angular velocity

// Function compute puts the GL modelview matrix into f.
// Functions advance_* do inertial motion for time dt.

void compute (char * buffer, std::size_t stride, const float (* x) [4], const float (* u) [4], unsigned count);
void advance_linear (float (* x) [4], const float (* v) [4], unsigned count, float dt);
void advance_angular (float (* u) [4], float (* w) [4], unsigned count, float dt);
void rotate (float (& u) [4], const float (& v) [4]);

// sincos: Argument x x * *, result sin(x) cos(x) sin(x) cos(x), range ([-(1/2)pi, (3/2)*pi]).
// arccos: Argument x x x x, result acos(x) acos(x) acos(x) acos(x), range [sqrt(2)/2, 1].

v4f sincos (v4f x);
v4f arccos (v4f x);

#endif
