// -*- C++ -*-

// Copyright 2012-2019 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef graphics_h
#define graphics_h

#include <cstddef>
#include <cstdint>
#include <GL/gl.h>

void clear ();

unsigned make_vao (
  unsigned N, const float (* vertices) [4], const std::uint8_t (* indices) [6]);

struct object_data_t
{
  GLfloat d [4];      // vec4,  diffuse reflection
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
  inline object_data_t & operator [] (std::size_t index) const
  {
    return * ((object_data_t *) (m_begin + index * m_stride));
  }

  ~uniform_buffer_t ();
  bool initialize ();
  void bind ();
  void update ();
private:
  std::uint32_t m_size;
  std::uint32_t m_stride;
  void * m_memory;
  char * m_begin;
  GLuint m_id;
};

struct program_t
{
  GLuint id;
  uniform_buffer_t uniform_buffer;
  bool initialize ();
  void set_view (const float (& view) [4],
                 int width, int height,
                 float fog_near, float fog_far,
                 float line_width, float line_margin);
};

bool initialize_graphics (program_t & program);

void paint (unsigned N,
            unsigned vao_id,
            GLuint uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset);

#endif
