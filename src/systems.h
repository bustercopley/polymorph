// -*- C++ -*-

#ifndef systems_h
#define systems_h

#include <cstdint>

enum system_select_t {
  tetrahedral, dual_tetrahedral,
  octahedral, dual_octahedral,
  icosahedral, dual_icosahedral,
  system_count
};

void initialize_systems (float (& abc) [system_count] [8] [4],
                         float (& xyz) [system_count] [3] [4],
                         unsigned (& N) [system_count],
                         unsigned (& vao_ids) [system_count]);

#endif
