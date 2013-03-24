// -*- C++ -*-

#ifndef markov_h
#define markov_h

#include "systems.h"

struct rng_t;

struct polyhedron_select_t
{
  system_select_t system;
  unsigned point;
};

void transition (rng_t & rng, float (& u) [4], polyhedron_select_t & current, unsigned & starting_point);

#endif
