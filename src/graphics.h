// -*- C++ -*-

#ifndef graphics_h
#define graphics_h

#include "vector.h"
#include <cstdint>

void clear ();

unsigned make_vao (unsigned N, const float (* vertices) [4], const std::uint8_t (* indices) [6]);

namespace uniforms
{
  enum index_t {
    p, l, g, m, r, d, s, f, e, count
  };
  extern const char * names [];
}

struct program_t
{
  std::uint32_t id;
  std::uint32_t uniform_locations [uniforms::count];
  bool initialize (v4f view, unsigned gshader_resource_id);
};

bool initialize_programs (program_t (& programs) [3], v4f view);

void paint (float radius,
            const float (& modelview_matrix) [16],
            const float (& rgba) [4],
            const float (& abc0) [4],
            unsigned N,
            unsigned vao_id,
            const program_t & program);

#endif
