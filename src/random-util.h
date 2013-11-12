// -*- C++ -*-

#ifndef random_util_h
#define random_util_h

#include "random.h"
#include "vector.h"

double get_double (rng_t & rng, double a, double b);
v4f get_vector_in_box (rng_t & rng);
v4f get_vector_in_ball (rng_t & rng, float radius);

#endif
