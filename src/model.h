// -*- C++ -*-

#ifndef model_h
#define model_h

#include "systems.h"
#include "random.h"
#include "kdtree.h"
#include "graphics.h"
#include "vector.h"

struct object_t
{
  float r, m, l;
  float hue, value, saturation;
  float generator_position;
  unsigned locus_begin, locus_end;
  system_select_t system_select;
  unsigned program_select;
};

struct model_t
{
  ~model_t ();
  model_t () : memory (nullptr), capacity (0) { }

  bool initialize (uint64_t seed, int width, int height);
  void proceed ();
  void draw ();
private:
  void set_capacity (unsigned new_capacity);
  void add_object (float phase, v4f frustum);

  float walls [6] [2] [4];
  float masks [6] [2] [4];
  float abc [system_count] [8] [4];
  float xyz [system_count] [3] [4];

  float max_radius;
  float animation_time_lo;

  void * memory;
  object_t * objects;
  unsigned * zorder_index;
  unsigned * kdtree_index;
  float (* x) [4];
  float (* v) [4];
  float (* u) [4];
  float (* w) [4];

  unsigned capacity;
  unsigned count;
  unsigned animation_time_hi;
  unsigned primitive_count [system_count];
  std::uint32_t vao_ids [system_count];

  program_t programs [3];
  kdtree_t kdtree;
  rng_t rng;
};

#endif
