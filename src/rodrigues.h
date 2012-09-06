// -*- C++ -*-

#ifndef rodrigues_h
#define rodrigues_h

// State: three-dimensional vectors x, u, v, w.
//   x: position (defined in point_t)
//   v: velocity
//   u: angular position
//   w: angular velocity

// Function compute puts the GL modelview matrix into f.
// Functions advance_* do inertial motion for time dt.

void compute (float (& f) [16], const float (& x) [4], const double (& u) [4], float r);
void advance_linear (float (* x) [4], const float (* v) [4], unsigned count, float dt);
void advance_angular (double (* u) [4], float (* w) [4], unsigned count, float dt);

#endif
