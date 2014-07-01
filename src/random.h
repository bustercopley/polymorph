// -*- C++ -*-

#ifndef random_h
#define random_h

#include <cstdint>

struct rng_t {
  rng_t () = default;
  void initialize (std::uint64_t seed);
  std::uint64_t get ();
private:
  std::uint64_t state;
  rng_t (const rng_t &) = delete;
  rng_t & operator = (const rng_t &) = delete;
};

#endif
