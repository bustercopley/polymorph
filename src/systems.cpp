#include "mswin.h"
#include "systems.h"
#include "nodes/system.h"
#include "graphics.h"
#include "vector.h"
#include "memory.h"
#include "compiler.h"
#include "aligned-arrays.h"
#include <cassert>

namespace
{
  NOINLINE
  auto append (float (* & out) [4], const float (* from) [3], unsigned count) -> float (*) [4]
  {
    count &= ~1; // Declare that count is even.
    for (unsigned n = 0; n != count; ++ n) {
      out [n] [0] = from [n] [0];
      out [n] [1] = from [n] [1];
      out [n] [2] = from [n] [2];
      out [n] [3] = 0.0f;
    }
    float (* result) [4] = out;
    out += count;
    return result;
  }

  NOINLINE
  void assign (float (* out) [4], const float (* from) [3], unsigned count)
  {
    for (unsigned n = 0; n != count; ++ n) {
      out [n] [0] = from [n] [0];
      out [n] [1] = from [n] [1];
      out [n] [2] = from [n] [2];
      out [n] [3] = 0.0f;
    }
  }

  NOINLINE
  unsigned make_vao (float (* vertices) [4], std::uint8_t (* indices) [6],
                     unsigned p, unsigned q, unsigned r,
                     const float (* x) [3], const float (* y) [3], const float (* z) [3],
                     const std::uint8_t * P, const std::uint8_t * Q, const std::uint8_t * R,
                     const std::uint8_t * X, const std::uint8_t * Y, const std::uint8_t * Z)
  {
    unsigned  N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r);

    float (* out) [4] = vertices;
    append (out, x, N / p);
    append (out, y, N / q);
    append (out, z, N / r);

    for (unsigned n = 0; n != N; ++ n) {
      unsigned i = n;
      unsigned j = i [R];
      unsigned k = j [P];
      assert (i == k [Q]);
      indices [n] [0] = X [k];
      indices [n] [1] = Y [j] + N / p;
      indices [n] [2] = Z [j] + N / p + N / q;
      indices [n] [3] = X [i];
      indices [n] [4] = Y [i] + N / p;
      indices [n] [5] = Z [k] + N / p + N / q;
    }
    return data_to_vao_id (N, vertices, indices);
  }

  template <unsigned q, unsigned r>
  void initialize_system (float (* vertices) [4], std::uint8_t (* indices) [6],
                          float (& abc) [system_count] [8] [4],
                          float (& xyz) [system_count] [3] [4],
                          unsigned (& primitive_count) [system_count],
                          unsigned (& vao_ids) [system_count],
                          const system_t <q, r> & system,
                          system_select_t select)
  {
    assign (abc [select], system.g, 8);
    assign (& xyz [select] [0], system.x, 1);
    assign (& xyz [select] [1], system.y, 1);
    assign (& xyz [select] [2], system.z, 1);
    primitive_count [select] = system.N;
    vao_ids [select] = make_vao (vertices, indices,
                                 2, q, r,
                                 system.x, system.y, system.z,
                                 system.P, system.Q, system.R,
                                 system.X, system.Y, system.Z);
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

  void  * memory = nullptr;
  unsigned capacity = 0;
  float (* vertices) [4] = nullptr;
  std::uint8_t (* indices) [6] = nullptr;
  reallocate_aligned_arrays (memory, capacity, i.N + 2, & vertices, & indices);

  initialize_system (vertices, indices, abc, xyz, primitive_count, vao_ids, t, tetrahedral);
  initialize_system (vertices, indices, abc, xyz, primitive_count, vao_ids, o, octahedral);
  initialize_system (vertices, indices, abc, xyz, primitive_count, vao_ids, i, icosahedral);

  deallocate (memory);
}
