// -*- C++ -*-

#ifndef rodrigues_h
#define rodrigues_h

#include "vector.h"
#include <cstddef>
#include <cstdint>

// State: three-dimensional vectors x, u, v, w.
//   x: position (defined in point_t)
//   v: velocity
//   u: angular position
//   w: angular velocity

// compute: put the GL modelview matrix into f.
// advance_*: do inertial motion for one time unit.
// rotate: bch (u, v) for small v (roughly, |v| <= pi/4).

void compute (char * buffer, std::size_t stride, const float (* x) [4], const float (* u) [4], unsigned * object_order, unsigned count);
void advance_linear (float (* x) [4], const float (* v) [4], unsigned count);
void advance_angular (float (* u) [4], float (* w) [4], unsigned count);
v4f rotate (v4f u, v4f v);

// sincos: argument x x * *, result sin(x) cos(x) sin(x) cos(x), range [-pi/2, pi/2].
// arccos: argument x x x x, result acos(x) acos(x) acos(x) acos(x), range [+0x1.8c97f0P-1f, +0x1.fb5486P-1f] ([0.774596691, 0.990879238])

v4f sincos (v4f x);
v4f arccos (v4f x);

#endif
