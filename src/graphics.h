// -*- C++ -*-

#ifndef graphics_h
#define graphics_h

#include <cstddef>
#include "mswin.h"
#include <GL/gl.h>

void clear ();

unsigned make_vao (unsigned N, const float (* vertices) [4], const unsigned (* indices) [6]);

namespace uniforms
{
  enum index_t {
    p,  // mat4,  projection matrix
    q,  // vec3,  geometry shader parameters
    f,  // vec3,  light position
    l,  // vec3,  fragment shader parameters
    a,  // vec3,  ambient reflection
    b,  // vec3,  background
    c,  // vec3,  line colour
    count
  };
}

// Warning: we make use of the fact that the names are all one letter long.
#define UNIFORM_NAME_STRINGS "p\0" "q\0" "f\0" "l\0" "a\0" "b\0" "c\0";

struct program_t
{
  GLuint id;
  GLuint uniform_locations [uniforms::count];
  bool initialize ();
  void set_view (const float (& view) [4],
                 float width, float height,
                 const float (& background) [3],
                 const float (& ambient) [3],
                 const float (& line_color) [3],
                 float fog_near, float fog_far,
                 float line_width_extra, float line_sharpness);
};

struct uniform_block_t
{
  GLfloat d [4];      // vec4,  diffuse reflectance
  GLfloat g [4];      // vec4,  uniform coefficients
  GLfloat m [4] [4];  // mat4,  modelview matrix
  GLuint s;           // bool,  snub?
};

struct uniform_buffer_t
{
  inline uniform_buffer_t () : m_memory (nullptr) { };
  inline std::size_t count () const { return m_size / m_stride; }
  inline std::size_t stride () const { return m_stride; }
  inline GLuint id () const { return m_id; }
  inline uniform_block_t & operator [] (std::size_t index) const
  {
    return * ((uniform_block_t *) (m_begin + index * m_stride));
  }

  ~uniform_buffer_t ();
  bool initialize ();
  void update ();
private:
  std::size_t m_size;
  void * m_memory;
  char * m_begin;
  std::ptrdiff_t m_stride;
  GLuint m_id;
};

bool initialize_graphics (program_t & program);

void paint (unsigned N,
            unsigned vao_id,
            GLuint uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset);

#endif
