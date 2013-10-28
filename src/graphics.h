// -*- C++ -*-

#ifndef graphics_h
#define graphics_h

#include "vector.h"
#include "systems.h"
#include <cstdint>

void clear ();

unsigned make_vao (unsigned N, const float (* vertices) [4], const std::uint8_t (* indices) [6]);

namespace uniforms
{
  enum index_t {
    p, // mat4,      projection matrix
    l, // vec3,      light position
    s, // vec3,      specular reflectance
    f, // float [2], fog coefficients
    count
  };
}

struct program_t
{
  std::uint32_t id;
  std::uint32_t uniform_locations [uniforms::count];
  bool initialize (v4f view, unsigned gshader_resource_id);
};

struct uniform_block_t
{
  float m [4] [4]; // mat4,    modelview matrix
  float g [4];     // vec4,    uniform coefficients
  float h [3] [4]; // vec4[3], triangle altitudes
  float d [4];     // vec4,    diffuse reflectance
  float r [4];     // float,   circumradius
};

struct uniform_buffer_t
{
  ~uniform_buffer_t ();
  inline uniform_buffer_t () : m_begin (0) { };
  bool initialize ();
  inline std::size_t count () const { return m_size / m_stride; }
  inline std::size_t stride () const { return m_stride; }
  inline std::size_t id () const { return m_id; }
  inline uniform_block_t & operator [] (std::size_t index) const
  {
    return * ((uniform_block_t *) (m_begin + index * m_stride));
  }
  void update ();
private:
  std::size_t m_size;
  void * m_memory;
  char * m_begin;
  std::ptrdiff_t m_stride;
  std::uint32_t m_id;
};

bool initialize_programs (program_t (& programs) [2], v4f view);

void paint (unsigned N,
            unsigned vao_id,
            const program_t & program,
            std::int32_t uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset);

#endif
