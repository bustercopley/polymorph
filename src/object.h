// -*- C++ -*-

#ifndef object_h
#define object_h

#include "rodrigues.h"
#include "kdtree.h"

struct system_ref_t;
struct system_repository_t;
struct frustum_t;
struct rng_t;

struct object_t
{
  float r, l, m;
  float hue, intensity, saturation;
  float generator_position;
  int chirality;
  unsigned locus_begin, locus_end;
  unsigned display_list;
  system_ref_t const * system_ref;

  void start (rng_t & rng);
  void update_appearance (float animation_time, rng_t & rng);
};

#endif
