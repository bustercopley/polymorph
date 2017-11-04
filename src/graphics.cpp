// Copyright 2012-2017 Richard Copley
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

#include "mswin.h"
#include "graphics.h"
#include "compiler.h"
#include "memory.h"
#include "resources.h"
#include "glinit.h"
#include "vector.h"
#include "print.h"
#include "glcheck.h"

#include <cstddef>
#include <cstdint>

#define UNIFORM_BINDING_INDEX_START 0

#if GLCHECK_ENABLED && PRINT_ENABLED
inline void gl_check (const char * file, int line) {
  GLint er = glGetError ();
  if (GLCHECK_TRACE || er) {
    std::cout << file << ":" << line << ":";
    if (er) std::cout << "error " << er;
    else std::cout << "OK";
    std::cout << std::endl;
  }
  if (er) {
    ::ExitProcess (er);
  }
}
#define GLCHECK gl_check (__FILE__, __LINE__)
#elif GLCHECK_ENABLED
#define GLCHECK do { GLint er = glGetError (); if (er) ::ExitProcess (er); } while (false)
#else
#define GLCHECK do { } while (false)
#endif

#if PRINT_ENABLED
#define PRINT_INFO_LOG(a, b, c) print_info_log (a, b, c)
inline void print_info_log (GLuint object,
                            PFNGLGETSHADERIVPROC glGet__iv,
                            PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
  GLint log_length;
  glGet__iv (object, GL_INFO_LOG_LENGTH, & log_length); GLCHECK;
  char * log = (char *) allocate (log_length + 1);
  if (log) {
    glGet__InfoLog (object, log_length, NULL, log); GLCHECK;
    log [log_length] = '\0';
    if (log [0]) print (log);
    deallocate (log);
  }
}
#else
#define PRINT_INFO_LOG(a, b, c) do { } while (0)
#endif

#if GLDEBUG_ENABLED
void APIENTRY debug_message_callback (GLenum source, GLenum type, GLuint id, GLenum severity,
                                      GLsizei, const GLchar * message, const void *)
{
  // Filter out anything that isn't an error or performance issue.
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

  std::cout << std::hex << "Source ";
  switch (source) {
  case /* 0x8246 */ GL_DEBUG_SOURCE_API:              std::cout << "API"; break;
  case /* 0x8247 */ GL_DEBUG_SOURCE_WINDOW_SYSTEM:    std::cout << "WINDOW_SYSTEM"; break;
  case /* 0x8248 */ GL_DEBUG_SOURCE_SHADER_COMPILER:  std::cout << "SHADER_COMPILER"; break;
  case /* 0x8249 */ GL_DEBUG_SOURCE_THIRD_PARTY:      std::cout << "THIRD_PARTY"; break;
  case /* 0x824A */ GL_DEBUG_SOURCE_APPLICATION:      std::cout << "APPLICATION"; break;
  case /* 0x824B */ GL_DEBUG_SOURCE_OTHER:            std::cout << "OTHER"; break;
  default:                                            std::cout << "unknown (0x" << source << ")"; break;
  }
  std::cout << ", type ";
  switch (type) {
  case /* 0x824C */ GL_DEBUG_TYPE_ERROR:                std::cout << "ERROR"; break;
  case /* 0x824D */ GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:  std::cout << "DEPRECATED_BEHAVIOR"; break;
  case /* 0x824E */ GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:   std::cout << "UNDEFINED_BEHAVIOR"; break;
  case /* 0x824F */ GL_DEBUG_TYPE_PORTABILITY:          std::cout << "PORTABILITY"; break;
  case /* 0x8250 */ GL_DEBUG_TYPE_PERFORMANCE:          std::cout << "PERFORMANCE"; break;
  case /* 0x8251 */ GL_DEBUG_TYPE_OTHER:                std::cout << "OTHER"; break;
  case /* 0x8268 */ GL_DEBUG_TYPE_MARKER:               std::cout << "MARKER"; break;
  case /* 0x8269 */ GL_DEBUG_TYPE_PUSH_GROUP:           std::cout << "PUSH_GROUP"; break;
  case /* 0x826A */ GL_DEBUG_TYPE_POP_GROUP:            std::cout << "POP_GROUP"; break;
  default:                                              std::cout << "unknown (0x" << type << ")"; break;
  }
  std::cout << ", severity ";
  switch (severity) {
  case /* 0x826B */ GL_DEBUG_SEVERITY_NOTIFICATION:  std::cout << "NOTIFICATION"; break;
  case /* 0x9146 */ GL_DEBUG_SEVERITY_HIGH:          std::cout << "HIGH"; break;
  case /* 0x9147 */ GL_DEBUG_SEVERITY_MEDIUM:        std::cout << "MEDIUM"; break;
  case /* 0x9148 */ GL_DEBUG_SEVERITY_LOW:           std::cout << "LOW"; break;
  default:                                           std::cout << "unknown (0x" << severity << ")"; break;
  }
  std::cout << std::dec << ", id " << id << ", message \"" << message << "\"" << std::endl;
}
#endif

GLuint make_shader (GLenum type, int resource_id)
{
  const char * text;
  GLint size;
  get_resource_data (resource_id, text, size);

  GLint id = glCreateShader (type); GLCHECK;
  glShaderSource (id, 1, & text, & size); GLCHECK;
  glCompileShader (id); GLCHECK;
  GLint status = 0;
  glGetShaderiv (id, GL_COMPILE_STATUS, & status); GLCHECK;
#if PRINT_ENABLED
  switch (resource_id) {
  case IDR_VERTEX_SHADER: std::cout << "Vertex "; break;
  case IDR_FRAGMENT_SHADER: std::cout << "Fragment "; break;
  case IDR_GEOMETRY_SHADER: std::cout << "Geometry "; break;
  default: ;
  }
  std::cout << "Shader compilation " << (status ? "succeeded." : "failed.") << std::endl;
  PRINT_INFO_LOG (id, glGetShaderiv, glGetShaderInfoLog); GLCHECK;
#endif
  return status ? id : 0;
}

static const int attribute_id_x = 0;

unsigned make_vao (unsigned N, const float (* vertices) [4], const std::uint8_t (* indices) [6])
{
  unsigned vao_id;
  glGenVertexArrays (1, & vao_id); GLCHECK;
  glBindVertexArray (vao_id); GLCHECK;
  GLuint buffer_ids [2];
  glGenBuffers (2, buffer_ids); GLCHECK;
  glBindBuffer (GL_ARRAY_BUFFER, buffer_ids [0]); GLCHECK;
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, buffer_ids [1]); GLCHECK;
  glBufferData (GL_ARRAY_BUFFER, (N + 2) * 4 * sizeof (float), vertices, GL_STATIC_DRAW); GLCHECK;
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * N * sizeof (std::uint8_t), indices, GL_STATIC_DRAW); GLCHECK;
  glVertexAttribPointer (attribute_id_x, 4, GL_FLOAT, GL_FALSE, 0, 0); GLCHECK;
  glEnableVertexAttribArray (attribute_id_x); GLCHECK;
  return vao_id;
}

// Align x upwards to given alignment (which must be a power of 2). Store the result in y.
template <typename Dest, typename Source>
inline void align_up (Dest & y, Source x, std::intptr_t alignment)
{
  y = (Dest) ((((std::intptr_t) (x)) + (alignment - 1)) & - alignment);
}

uniform_buffer_t::~uniform_buffer_t ()
{
  //glDeleteBuffers (1, & m_id); GLCHECK;
  //deallocate (m_memory);
}

bool uniform_buffer_t::initialize ()
{
  GLint max_size, align;
  glGetIntegerv (GL_MAX_UNIFORM_BLOCK_SIZE, & max_size); GLCHECK;
  glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, & align); GLCHECK;
  // Align client-side buffer to at least 64 bytes, to avoid straddling cache lines.
  align = align > 64 ? align : 64;
  m_size = max_size;
  m_memory = allocate (m_size + align);
  if (! m_memory) return false;
  align_up (m_begin, m_memory, align);
  align_up (m_stride, sizeof (object_data_t), align);
  glGenBuffers (1, & m_id); GLCHECK;
  return true;
}

void uniform_buffer_t::bind ()
{
  glBindBuffer (GL_UNIFORM_BUFFER, m_id); GLCHECK;
}

void uniform_buffer_t::update ()
{
  // Create a buffer, orphaning any previous buffer.
  glBufferData (GL_UNIFORM_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW); GLCHECK;
  // Upload the buffer data.
  glBufferSubData (GL_UNIFORM_BUFFER, 0, m_size, m_begin); GLCHECK;
}

bool initialize_graphics (program_t & program)
{
  glEnable (GL_DEPTH_CLAMP); GLCHECK;
  glEnable (GL_CULL_FACE); GLCHECK;
  glEnable (GL_BLEND); GLCHECK;
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLCHECK;

#if GLDEBUG_ENABLED
  glEnable (GL_DEBUG_OUTPUT); GLCHECK;
  glDebugMessageCallback (debug_message_callback, nullptr); GLCHECK;
  glDebugMessageControl (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE); GLCHECK;
#endif

  return program.initialize ();
}

void program_t::set_view (const float (& view) [4],
                          int width, int height,
                          float fog_near, float fog_far,
                          float line_width_extra, float line_sharpness)
{
  glViewport (0, 0, width, height); GLCHECK;

  float x1 = view [0];  // x coord of right edge of screen (and of front-right edge of tank)
  float y1 = view [1];  // y coord of top edge of screen (and of front-top edge of tank)
  float z1 = view [2];  // z coord of screen (front of tank) (a negative number)
  float z2 = view [3];  // z coord of back of tank (a negative number) (|z2| > |z1|)
  float zd = z1 - z2;   // tank depth

  // Fog is linear in z-distance.
  float fogn = 1.0f - fog_near;
  float fogf = 1.0f - fog_far;
  float fogd = fogn - fogf;
  float fog1 = fogd / zd;
  float fog0 = fogf - fog1 * z2;

  float line0 = -0.5f * line_width_extra * line_sharpness;
  float line1 = line_sharpness;

  // Data blocks in layout std140.

  struct fragment_data_t
  {
    GLfloat a [4];      // vec3,      ambient reflection (rgb)
    GLfloat b [4];      // vec3,      background (rgb)
    GLfloat c [4];      // vec4,      line colour (rgba)
    GLfloat r [4];      // vec4,      xyz: specular reflection (rgb); w: exponent
    GLfloat f [4];      // vec4,      coefficients for line width and fog
    GLfloat l [4] [4];  // vec3 [4],  light positions
  };

  struct geometry_data_t
  {
    GLfloat p [4] [4];  // mat4,      projection matrix
    GLfloat q [4];      // vec3,      pixel scale of normalized device coordinates
  };

  ALIGNED16 fragment_data_t fragment_data = {
    { 0.02f, 0.02f, 0.02f, 0.00f, },
    { 0.00f, 0.00f, 0.00f, 0.00f, },
    { 0.00f, 0.00f, 0.00f, 1.00f, },
    { 0.25f, 0.25f, 0.25f, 32.0f, },
    { line0, line1, fog0, fog1, },
    {
      { -0.6f * x1, -0.2f * y1, z1 + y1, 0.0f, },
      { -0.2f * x1, +0.6f * y1, z1 + x1, 0.0f, },
      { +0.2f * x1, -0.6f * y1, z1 + y1, 0.0f, },
      { +0.6f * x1, +0.2f * y1, z1 + x1, 0.0f, },
    },
  };

  ALIGNED16 geometry_data_t geometry_data = {
    {
      // Save a few instructions by using a simplified projection matrix
      // which doesn't calculate the z component of the clip coordinates.
      // We don't have any use for that component since we disable near
      // and far plane clipping and we don't have a depth buffer.
      { -z1/x1,     0,       0,       0, },
      {    0,    -z1/y1,     0,       0, },
      {    0,       0,       0,      -1, },
      {    0,       0,       0,       0, },
    },
    { float (width >> 1), float (height >> 1), 0, 0, },
  };

  // Descriptions of the data blocks.

  struct block_descriptor_t
  {
    const char * name;
    GLvoid * data;
    GLsizeiptr size;
  };

  block_descriptor_t blocks [] = {
    { "F", & fragment_data, sizeof fragment_data, },
    { "G", & geometry_data, sizeof geometry_data, },
  };
  GLuint block_count = sizeof blocks / sizeof blocks [0];

  // Create a uniform buffer big enough for all the data blocks.

  GLsizeiptr max_block_size = 0;
  for (std::size_t n = 0; n != block_count; ++ n)
    if (max_block_size < blocks [n].size)
      max_block_size = blocks [n].size;

  const float (& background) [4] = fragment_data.b;
  glClearColor (background [0], background [1], background [2], 0.0f); GLCHECK;

  GLint align;
  glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, & align); GLCHECK;

  GLuint buffer_size, buffer_stride;
  align_up (buffer_stride, max_block_size, align);
  buffer_size = block_count * buffer_stride;

  GLuint buf_id;
  glGenBuffers (1, & buf_id); GLCHECK;
  glBindBuffer (GL_UNIFORM_BUFFER, buf_id); GLCHECK;
  glBufferData (GL_UNIFORM_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW); GLCHECK;

  // Copy each data block to a buffer range and bind the range to the named shader uniform block.

  for (std::size_t n = 0; n != block_count; ++ n) {
    glBufferSubData (GL_UNIFORM_BUFFER, n * buffer_stride, blocks [n].size, blocks [n].data); GLCHECK;
    GLuint block_index = glGetUniformBlockIndex (id, blocks [n].name); GLCHECK;
    GLuint binding_index = UNIFORM_BINDING_INDEX_START + n + 1;
    glUniformBlockBinding (id, block_index, binding_index); GLCHECK;
    glBindBufferRange (GL_UNIFORM_BUFFER, binding_index, buf_id, n * buffer_stride, blocks [n].size); GLCHECK;
  }

  uniform_buffer.bind ();
}

bool program_t::initialize ()
{
  if (! uniform_buffer.initialize ()) return false;

  GLint status = 0;
  GLuint vshader_id = make_shader (GL_VERTEX_SHADER, IDR_VERTEX_SHADER);
  GLuint gshader_id = make_shader (GL_GEOMETRY_SHADER, IDR_GEOMETRY_SHADER);
  GLuint fshader_id = make_shader (GL_FRAGMENT_SHADER, IDR_FRAGMENT_SHADER);
  if (vshader_id && gshader_id && fshader_id) {
    id = glCreateProgram (); GLCHECK;
    if (id) {
      glAttachShader (id, vshader_id); GLCHECK;
      glAttachShader (id, gshader_id); GLCHECK;
      glAttachShader (id, fshader_id); GLCHECK;
      glBindAttribLocation (id, attribute_id_x, "x"); GLCHECK;
      glLinkProgram (id); GLCHECK;
      glGetProgramiv (id, GL_LINK_STATUS, & status); GLCHECK;
#if PRINT_ENABLED
      std::cout << "Shader program linking " << (status ? "succeeded." : "failed.") << std::endl;
      PRINT_INFO_LOG (id, glGetProgramiv, glGetProgramInfoLog); GLCHECK;
#endif
      glDetachShader (id, fshader_id); GLCHECK;
      glDetachShader (id, gshader_id); GLCHECK;
      glDetachShader (id, vshader_id); GLCHECK;
    }
  }
  if (fshader_id) { glDeleteShader (fshader_id); GLCHECK; }
  if (gshader_id) { glDeleteShader (gshader_id); GLCHECK; }
  if (vshader_id) { glDeleteShader (vshader_id); GLCHECK; }

  if (! status) {
    //if (id) { glDeleteProgram (id); GLCHECK; }
    return false;
  }

  // Binding for Uniform block "H".
  GLuint object_uniform_block_index = glGetUniformBlockIndex (id, "H"); GLCHECK;
  glUniformBlockBinding (id, object_uniform_block_index, UNIFORM_BINDING_INDEX_START); GLCHECK;
  glUseProgram (id); GLCHECK;

  return true;
}

void clear ()
{
  glClear (GL_COLOR_BUFFER_BIT); GLCHECK;
}

void paint (unsigned N,
            unsigned vao_id,
            GLuint uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset)
{
  const unsigned block_size = sizeof (object_data_t);
  glBindVertexArray (vao_id); GLCHECK;
  glBindBufferRange (GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_START, uniform_buffer_id, uniform_buffer_offset, block_size); GLCHECK;
  glCullFace (GL_FRONT); GLCHECK;
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr); GLCHECK;
  glCullFace (GL_BACK); GLCHECK;
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr); GLCHECK;
}
