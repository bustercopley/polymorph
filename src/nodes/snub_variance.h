// -*- C++ -*-

#ifndef snub_variance_h
#define snub_variance_h

#include <cstdint>

long double snub_variance (const float (* u) [3], const float (* v) [3], const float (* w) [3],
                           const float (& g7) [3], unsigned Np, const uint8_t * x, const uint8_t (* s) [4]);

#endif
