// -*- C++ -*-

#ifndef random_h
#define random_h

#include <cstdint>
#include "vector.h"

struct rng_t {
  rng_t () = default;
  void initialize (uint64_t seed);
  uint64_t get ();
  double get_double (double a, double b);
  v4f get_vector_in_box ();
  v4f get_vector_in_ball (float radius);
private:
  uint64_t state;
  rng_t (const rng_t &) = delete;
  rng_t & operator = (const rng_t &) = delete;
};

#endif
