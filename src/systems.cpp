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

    if (((unsigned) select) & 1) {
      for (unsigned n = 0; n != 8; ++ n) {
        v4f t = _mm_loadu_ps (abc_in [n]);
        store4f (abc [select] [n], _mm_shuffle_ps (t, t, SHUFFLE (0, 2, 1, 3)));
      }
    }
    else {
      assign (abc [select], abc_in, 8);
    }
    assign (& xyz [select] [0], xyz_in + indices_in [0] [0], 1);
    assign (& xyz [select] [1], xyz_in + indices_in [0] [4], 1);
    assign (& xyz [select] [2], xyz_in + indices_in [0] [2], 1);

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
  system_t <4, 3> const & o (* reinterpret_cast <system_t <4, 3> const *> (& t + 1));
  system_t <5, 3> const & i (* reinterpret_cast <system_t <5, 3> const *> (& o + 1));

  initialize_system (abc, xyz, primitive_count, vao_ids, t.abc, t.xyz, t.indices [0], tetrahedral, 3, 3);
  initialize_system (abc, xyz, primitive_count, vao_ids, t.abc, t.xyz, t.indices [1], dual_tetrahedral, 3, 3);
  initialize_system (abc, xyz, primitive_count, vao_ids, o.abc, o.xyz, o.indices [0], octahedral, 4, 3);
  initialize_system (abc, xyz, primitive_count, vao_ids, o.abc, o.xyz, o.indices [1], dual_octahedral, 4, 3);
  initialize_system (abc, xyz, primitive_count, vao_ids, i.abc, i.xyz, i.indices [0], icosahedral, 5, 3);
  initialize_system (abc, xyz, primitive_count, vao_ids, i.abc, i.xyz, i.indices [1], dual_icosahedral, 5, 3);
}
