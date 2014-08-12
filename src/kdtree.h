// -*- C++ -*-

#ifndef kdtree_h
#define kdtree_h

#include <cstddef>

struct object_t;

// The behaviour of kdtree_t::compute is undefined if count is zero.

struct kdtree_t
{
  kdtree_t () : split (nullptr), capacity (0) { }
  ~kdtree_t ();
  bool compute (unsigned * index, const float (* x) [4], unsigned count);
  void search (unsigned * index, const float (* x) [4], unsigned count,
               const float (* walls) [2] [4],  float max_radius,
               object_t * objects, const float (* r), float (* v) [4], float (* w) [4]) const;
private:
  float * split;
  std::size_t capacity;
  unsigned first_leaf;
};

#endif
