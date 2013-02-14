#include "mswin.h"
#include "systems.h"
#include "nodes/system.h"
#include "graphics.h"
#include "memory.h"
#include "compiler.h"
#include "aligned-arrays.h"
#include <cstdint>

namespace
{
  NOINLINE
  void assign (float (* out) [4], const float (* from) [4], unsigned count)
  {
    copy_memory (out, from, count * 4 * sizeof (float));
  }

  void initialize_system (float (& abc) [system_count] [8] [4],
                          float (& xyz) [system_count] [3] [4],
                          unsigned (& primitive_count) [system_count],
                          unsigned (& vao_ids) [system_count],
                          const float (& abc_in) [8] [4],
                          const float (* xyz_in) [4],
                          const std::uint8_t (* indices_in) [6],
                          system_select_t select,
                          unsigned q, unsigned r)
  {
    unsigned p = 2;
    unsigned N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r);

    assign (abc [select], abc_in, 8);
    assign (& xyz [select] [0], xyz_in, 1);
    assign (& xyz [select] [1], xyz_in + N / p, 1);
    assign (& xyz [select] [2], xyz_in + N / p + N / q, 1);

    primitive_count [select] = N;
    vao_ids [select] = make_vao (N, xyz_in, indices_in);
  }
}

void initialize_systems (float (& abc) [system_count] [8] [4],
                         float (& xyz) [system_count] [3] [4],
                         unsigned (& primitive_count) [system_count],
                         unsigned (& vao_ids) [system_count])
{
  const void * data = get_resource_data (256, nullptr);
  system_t <3, 3> const & t (* reinterpret_cast <system_t <3, 3> const *> (data));
  system_t <3, 4> const & o (* reinterpret_cast <system_t <3, 4> const *> (& t + 1));
  system_t <3, 5> const & i (* reinterpret_cast <system_t <3, 5> const *> (& o + 1));

  initialize_system (abc, xyz, primitive_count, vao_ids, t.abc, t.xyz, t.indices, tetrahedral, 3, 3);
  initialize_system (abc, xyz, primitive_count, vao_ids, o.abc, o.xyz, o.indices, octahedral, 3, 4);
  initialize_system (abc, xyz, primitive_count, vao_ids, i.abc, i.xyz, i.indices, icosahedral, 3, 5);
}
