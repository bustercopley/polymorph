// -*- C++ -*-

#ifndef bounce_h
#define bounce_h

struct object_t;
struct plane_t;

void bounce (unsigned ix, unsigned iy, object_t * objects,
             const float (* x) [4], float (* v) [4], float (* w) [4]);

void bounce (const float (& plane) [2] [4], unsigned ix, object_t * objects,
             const float (* x) [4], float (* v) [4], float (* w) [4]);

#endif
