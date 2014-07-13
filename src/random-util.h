// -*- C++ -*-

#ifndef random_util_h
#define random_util_h

#include "random.h"
#include "vector.h"

float get_float (rng_t & rng, float a, float b);
v4f get_vector_in_box (rng_t & rng);
v4f get_vector_in_ball (rng_t & rng, float radius);

#endif
