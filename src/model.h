// -*- C++ -*-

#ifndef model_h
#define model_h

#include "compiler.h"
#include "memory.h"
#include "systems.h"
#include "random.h"
#include "kdtree.h"
#include "graphics.h"
#include "bump.h"
#include "vector.h"
#include "markov.h"

struct object_t
{
  float m, l;
  float hue;
  float animation_time;
  float locus_end [4];
  float locus_speed;
  unsigned starting_point;
  polyhedron_select_t target;
};

struct model_t
{
  ~model_t ();
  model_t ();

  bool initialize (uint64_t seed, int width, int height);
  void proceed ();
  void draw ();
private:
  bool set_capacity (unsigned new_capacity);
  void add_object (v4f view);
  void recalculate_locus (object_t & A);

  float walls [6] [2] [4] ALIGNED16;
  float abc [system_count] [8] [4] ALIGNED16;
  float xyz [system_count] [3] [4] ALIGNED16;
  float xyzinv [system_count] [3] [4] ALIGNED16;
  bumps_t bumps ALIGNED16;
  step_t step ALIGNED16;

  float max_radius;

  void * memory;
  object_t * objects;
  unsigned * kdtree_index;
  float (* r);      // circumradius
  float (* x) [4];  // position
  float (* v) [4];  // velocity
  float (* u) [4];  // angular position
  float (* w) [4];  // angular velocity
  float (* f) [16]; // modelview matrix
  float (* d) [4];  // diffuse colour

  unsigned capacity;
  unsigned count;
  unsigned primitive_count [system_count]; // = { 12, 24, 60, }
  std::uint32_t vao_ids [system_count];

  program_t programs [2];
  kdtree_t kdtree;
  rng_t rng;
};

#endif
