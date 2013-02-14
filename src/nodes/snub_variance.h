// -*- C++ -*-

#ifndef snub_variance_h
#define snub_variance_h

#include <cstdint>

long double snub_variance (const uint8_t * P, const uint8_t * Q, const uint8_t * R,
                           const uint8_t * X, const uint8_t * Y, const uint8_t * Z,
                           const float (* x) [4], const float (* y) [4], const float (* z) [4],
                           const float (& g7) [4], unsigned N);

#endif
