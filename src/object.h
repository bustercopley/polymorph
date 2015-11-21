// -*- C++ -*-

#ifndef object_h
#define object_h

#include "markov.h"

struct object_t
{
  float m, l, r;
  float hue;
  float animation_time;
  float locus_length;
  unsigned starting_point;
  polyhedron_select_t target;
};

#endif
