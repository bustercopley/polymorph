// -*- C++ -*-

#ifndef make_system_h
#define make_system_h

#include <cstdint>

void make_system (unsigned q, unsigned r, const float (& xyz_in) [3] [4], float (* nodes) [4], std::uint8_t (* indices) [6]);

#endif
