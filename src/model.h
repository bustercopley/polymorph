// -*- C++ -*-

#ifndef model_h
#define model_h

#include "system_ref.h"
#include "frustum.h"
#include "random.h"
#include "kdtree.h"
#include "object.h"

struct model_t {
  ~model_t ();
  void initialize (void * data, uint64_t seed, const view_t & frustum);
  void proceed ();
  void draw ();
  void set_capacity (unsigned new_capacity);
  void add_object (float phase);
  void ball_bounce (unsigned i, unsigned j);
  void wall_bounce (unsigned i, unsigned j);
private:
  float walls [6] [2] [4];

  void * memory;
  float (* x) [4];
  float (* v) [4];
  float (* u) [4];
  float (* w) [4];
  unsigned * zorder_index;
  unsigned * kdtree_index;
  object_t * objects;

  float max_radius;
  float animation_time;
  unsigned count;
  unsigned capacity;

  system_repository_t repository;
  rng_t rng;
  kdtree_t kdtree;
  view_t view;
};

#endif
