// -*- C++ -*-

#ifndef model_h
#define model_h

#include "compiler.h"
#include "random.h"
#include "kdtree.h"
#include "graphics.h"
#include "bump.h"
#include "markov.h"
#include "settings.h"
#include <cstdint>

struct object_t
{
  float m, l, r;
  float hue;
  float animation_time;
  float locus_length;
  unsigned starting_point;
  polyhedron_select_t target;
};

struct model_t
{
  ~model_t ();
  model_t ();

  bool initialize (std::uint64_t seed);
  bool start (int width, int height, const settings_t & settings);
  void draw_next ();
private:
  void nodraw_next ();
  bool set_capacity (std::size_t new_capacity);
  void add_object (const float (& view) [4], float temperature);
  void recalculate_locus (unsigned index);
  void draw (unsigned begin, unsigned count);

  ALIGNED16 float walls [6] [2] [4];
  ALIGNED16 float abc [system_count] [8] [4];
  ALIGNED16 float xyz [system_count] [3] [4];
  ALIGNED16 float xyzinvt [system_count] [3] [4];
  ALIGNED16 bumps_t bumps;
  ALIGNED16 step_t step;

  void * memory;
  object_t * objects;
  unsigned * object_order;
  unsigned * kdtree_index;
  float (* x) [4];  // position
  float (* v) [4];  // velocity
  float (* u) [4];  // angular position
  float (* w) [4];  // angular velocity
  float (* e) [4];  // locus end

  program_t program;
  kdtree_t kdtree;
  rng_t rng;

  float max_radius;
  float animation_speed_constant;

  std::size_t capacity;
  unsigned count;
  unsigned primitive_count [system_count]; // = { 12, 24, 60, }
  std::uint32_t vao_ids [system_count];
  uniform_buffer_t uniform_buffer;
};

#endif
