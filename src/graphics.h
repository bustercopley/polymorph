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
    p, // mat4,      projection matrix
    l, // vec3,      light position
    f, // float [2], fog coefficients
    w, // int,       pixel width
    h, // int,       pixel height
    count
  };
}

struct program_t
{
  GLuint id;
  GLuint uniform_locations [uniforms::count];
  bool initialize ();
};

struct uniform_block_t
{
  GLfloat d [4];      // vec4,  diffuse reflectance
  GLfloat g [4];      // vec4,  uniform coefficients
  GLfloat m [4] [4];  // mat4,  modelview matrix
  GLboolean s;        // bool,  snub?
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
void set_view (const float (& view) [4], int width, int height, GLuint * uniform_locations);

void paint (unsigned N,
            unsigned vao_id,
            GLuint uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset);

#endif
