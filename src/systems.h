// -*- C++ -*-

#ifndef systems_h
#define systems_h

#include <cstdint>

enum system_select_t {
  tetrahedral, octahedral, icosahedral,
  system_count,
  hexahedral = octahedral,
  dodecahedral = icosahedral
};

void initialize_systems (float (& abc) [system_count] [8] [4],
                         float (& xyz) [system_count] [3] [4],
                         unsigned (& N) [system_count],
                         unsigned (& vao_ids) [system_count]);

#endif
