// -*- C++ -*-

#ifndef snub_variance_h
#define snub_variance_h

#include <cstdint>
#include <utility>

std::pair <double, double>
snub_variance (const float (* xyz) [4],
               const std::uint8_t (* indices) [6],
               const float (& g7) [4], unsigned N);

#endif
