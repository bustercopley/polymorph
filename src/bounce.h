// -*- C++ -*-

#ifndef bounce_h
#define bounce_h

struct object_t;
struct plane_t;

void bounce (unsigned i, unsigned j, object_t * objects,
             const float (* x) [4], const float (* v) [4], float (* vd) [4],
             const float (* w) [4], float (* wd) [4]);

void bounce (const float (& plane) [2] [4], unsigned i, object_t * objects,
             const float (* x) [4], const float (* v) [4], float (* vd) [4],
             const float (* w) [4], float (* wd) [4]);

#endif
