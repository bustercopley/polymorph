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

#include "mswin.h"

#include "graphics.h"
#include "compiler.h"
#include "glcheck.h"
#include "glinit.h"
#include "memory.h"
#include "print.h"
#include "resources.h"
#include <algorithm>

// Uniform block binding indices and vertex attribute indices.
// These must match the bindings in shader layout qualifiers.
#define BLOCK_H 0
#define BLOCK_F 1
#define BLOCK_G 2
#define ATTRIB_X 0

#if GLCHECK_ENABLED && PRINT_ENABLED
inline void gl_check (const char * file, int line)
{
  GLint er = glGetError ();
  if (GLCHECK_TRACE || er) {
    std::cout << file << ":" << line << ":";
    if (er) {
      std::cout << "error " << er;
    }
    else {
      std::cout << "OK";
    }
    std::cout << std::endl;
  }
  if (er) {
    ::ExitProcess (er);
  }
}
#define GLCHECK gl_check (__FILE__, __LINE__)
#elif GLCHECK_ENABLED
#define GLCHECK                                                                \
  do {                                                                         \
    GLint er = glGetError ();                                                  \
    if (er)                                                                    \
      ::ExitProcess (er);                                                      \
  } while (false)
#else
#define GLCHECK do { } while (false)
#endif

#if PRINT_ENABLED
#define PRINT_INFO_LOG(a, b, c) print_info_log (a, b, c)
inline void print_info_log (GLuint object,
  PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
  GLint log_length;
  glGet__iv (object, GL_INFO_LOG_LENGTH, & log_length); GLCHECK;
  char * log = (char *) allocate (log_length + 1);
  if (log) {
    glGet__InfoLog (object, log_length, nullptr, log); GLCHECK;
    log [log_length] = '\0';
    if (log [0]) print (log);
    deallocate (log);
  }
}
#else
#define PRINT_INFO_LOG(a, b, c) do { } while (0)
#endif

#define GL_DEBUG_CASE(kind, thing)                                             \
  case GL_DEBUG_##kind##_##thing:                                              \
    std::cout << #thing;                                                       \
    break;

#if GLDEBUG_ENABLED
void APIENTRY debug_message_callback (GLenum source, GLenum type, GLuint id,
  GLenum severity, GLsizei, const GLchar * message, const void *)
{
  // Filter out anything that isn't an error or performance issue.
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

  std::cout << std::hex << "Source ";
  switch (source) {
    GL_DEBUG_CASE (SOURCE, API);
    GL_DEBUG_CASE (SOURCE, WINDOW_SYSTEM);
    GL_DEBUG_CASE (SOURCE, SHADER_COMPILER);
    GL_DEBUG_CASE (SOURCE, THIRD_PARTY);
    GL_DEBUG_CASE (SOURCE, APPLICATION);
    GL_DEBUG_CASE (SOURCE, OTHER);
  default: std::cout << "[" << source << "]"; break;
  }
  std::cout << ", type ";
  switch (type) {
    GL_DEBUG_CASE (TYPE, ERROR);
    GL_DEBUG_CASE (TYPE, DEPRECATED_BEHAVIOR);
    GL_DEBUG_CASE (TYPE, UNDEFINED_BEHAVIOR);
    GL_DEBUG_CASE (TYPE, PORTABILITY);
    GL_DEBUG_CASE (TYPE, PERFORMANCE);
    GL_DEBUG_CASE (TYPE, OTHER);
    GL_DEBUG_CASE (TYPE, MARKER);
    GL_DEBUG_CASE (TYPE, PUSH_GROUP);
    GL_DEBUG_CASE (TYPE, POP_GROUP);
  default: std::cout << "[" << type << "]"; break;
  }
  std::cout << ", severity ";
  switch (severity) {
    GL_DEBUG_CASE (SEVERITY, NOTIFICATION);
    GL_DEBUG_CASE (SEVERITY, HIGH);
    GL_DEBUG_CASE (SEVERITY, MEDIUM);
    GL_DEBUG_CASE (SEVERITY, LOW);
  default: std::cout << "[" << severity << "]"; break;
  }
  std::cout << std::dec << ", id " << id << ", message \"" << message << "\""
            << std::endl;
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
  std::cout << "Shader compilation " << (status ? "succeeded." : "failed.")
            << std::endl;
  PRINT_INFO_LOG (id, glGetShaderiv, glGetShaderInfoLog);
  GLCHECK;
#endif
  return status ? id : 0;
}

unsigned make_vao (
  unsigned N, const float (* vertices) [4], const std::uint8_t (* indices) [6])
{
  unsigned vao_id;
  glGenVertexArrays (1, & vao_id); GLCHECK;
  glBindVertexArray (vao_id); GLCHECK;
  GLuint buffer_ids [2];
  glGenBuffers (2, buffer_ids); GLCHECK;
  glBindBuffer (GL_ARRAY_BUFFER, buffer_ids [0]); GLCHECK;
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, buffer_ids [1]); GLCHECK;
  GLsizeiptr n = (N + 2) * 4 * sizeof (float);
  GLsizeiptr m = 6 * N * sizeof (std::uint8_t);
  glBufferData (GL_ARRAY_BUFFER, n, vertices, GL_STATIC_DRAW); GLCHECK;
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, m, indices, GL_STATIC_DRAW); GLCHECK;
  glVertexAttribPointer (ATTRIB_X, 4, GL_FLOAT, GL_FALSE, 0, 0); GLCHECK;
  glEnableVertexAttribArray (ATTRIB_X); GLCHECK;
  return vao_id;
}

// Align x upwards to given alignment (must be a power of 2). Store result in y.
template <typename Dest, typename Source>
inline void align_up (Dest & y, Source x, std::intptr_t alignment)
{
  y = (Dest) ((((std::intptr_t) (x)) + (alignment - 1)) & -alignment);
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

#if PRINT_ENABLED
  std::cout << "Max uniform block size " << max_size
            << "\nUniform buffer offset alignment " << align << std::endl;
#endif

  // Align client buffer to at least 64 bytes to avoid straddling cache lines.
  align = std::max (align, 64);
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
  // The buffer id is already bound to the GL_UNIFORM_BUFFER target.
  // Create a buffer, orphaning any previous buffer.
  glBufferData (GL_UNIFORM_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW); GLCHECK;
  // Upload the buffer data.
  glBufferSubData (GL_UNIFORM_BUFFER, 0, m_size, m_begin); GLCHECK;
}

bool initialize_graphics (program_t & program)
{
  glEnable (GL_CULL_FACE); GLCHECK;
  glEnable (GL_BLEND); GLCHECK;
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLCHECK;

#if GLDEBUG_ENABLED
  glEnable (GL_DEBUG_OUTPUT); GLCHECK;
  glDebugMessageCallback (debug_message_callback, nullptr); GLCHECK;
  glDebugMessageControl (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
    0, 0, GL_TRUE);
  GLCHECK;
#endif

  return program.initialize ();
}

void program_t::set_view (const float (& view) [4],
                          int width, int height,
                          float fog_near, float fog_far,
                          float line0, float line1)
{
  glViewport (0, 0, width, height); GLCHECK;

  float x1 = view [0];  // right edge of screen (front-right edge of tank)
  float y1 = view [1];  // top edge of screen (front-top edge of tank)
  float z1 = view [2];  // screen (front of tank) (a negative number)
  float z2 = view [3];  // back of tank (a negative number) (|z2| > |z1|)
  float zd = z1 - z2;   // tank depth

  // Fog is linear in z-distance.
  float fogn = 1.0f - fog_near;
  float fogf = 1.0f - fog_far;
  float fogd = fogn - fogf;
  float fog1 = fogd / zd;
  float fog0 = fogf - fog1 * z2;

  // Data blocks in layout std140.

  struct fragment_data_t
  {
    GLfloat a [4];      // vec3, ambient reflection (rgb)
    GLfloat b [4];      // vec3, background (rgb)
    GLfloat c [4];      // vec4, line colour (rgba)
    GLfloat r [4];      // vec4, xyz: specular reflection (rgb); w: exponent
    GLfloat f [4];      // vec4, coefficients for line width and fog
    GLfloat l [4] [4];  // vec3 [4], light positions
  };

  struct geometry_data_t
 {
    GLfloat p [4] [4];  // mat4, projection matrix
    GLfloat q [4];      // vec3, pixel size in normalized device coordinates
  };

  ALIGNED16 fragment_data_t fdata = {
    { 0.02f, 0.02f, 0.02f, 0.00f },
    { 0.00f, 0.00f, 0.00f, 0.00f },
    { 0.00f, 0.00f, 0.00f, 1.00f },
    { 0.25f, 0.25f, 0.25f, 32.0f },
    { line0, line1, fog0, fog1 },
    {
      { -0.6f * x1, -0.2f * y1, z1 + y1, 0.0f },
      { -0.2f * x1, +0.6f * y1, z1 + x1, 0.0f },
      { +0.2f * x1, -0.6f * y1, z1 + y1, 0.0f },
      { +0.6f * x1, +0.2f * y1, z1 + x1, 0.0f },
    },
  };

  ALIGNED16 geometry_data_t gdata = {
    {
      // Save a few instructions by using a simplified projection matrix which
      // doesn't calculate the z component of the clip coordinates. We don't
      // have any use for that component since we disable near and far plane
      // clipping and we don't have a depth buffer.
      { -z1/x1,     0,       0,       0 },
      {    0,    -z1/y1,     0,       0 },
      {    0,       0,       0,      -1 },
      {    0,       0,       0,       0 },
    },
    { float (width >> 1), float (height >> 1), 0, 0 },
  };

  // The shaders declare uniform blocks "F", "G" and "H", each with its own
  // distinct hardcoded binding index of the indexed target GL_UNIFORM_BUFFER.
  // "F" is declared in "fragment-shader.glsl", "G" in "geometry-shader.glsl",
  // and "H" in both "fragment-shader.glsl" and "geometry-shader.glsl". The
  // layouts of blocks "F", "G" and "H" match the structs fragment_data_t,
  // geometry_data_t and object_data_t.

  // Create one uniform buffer large enough to hold the data for "F" and "G".
  // This uniform buffer is never destroyed (the uniform buffer id is leaked).
  GLuint stride = std::max (sizeof fdata, sizeof gdata);
  GLint align;
  glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, & align); GLCHECK;
  align_up (stride, stride, align);
  GLuint buf;
  glGenBuffers (1, & buf); GLCHECK;
  glBindBuffer (GL_UNIFORM_BUFFER, buf); GLCHECK;
  glBufferData (GL_UNIFORM_BUFFER, 2 * stride, nullptr, GL_STATIC_DRAW);
  GLCHECK;

  // Load each block's data into its own disjoint subrange of the buffer.
  glBufferSubData (GL_UNIFORM_BUFFER, 0 * stride, sizeof fdata, & fdata);
  GLCHECK;
  glBufferSubData (GL_UNIFORM_BUFFER, 1 * stride, sizeof gdata, & gdata);
  GLCHECK;

  // Bind "F" and "G" to the disjoint subranges. These indexed bindings remain
  // in place for the lifetime of the shader program.
  glBindBufferRange (GL_UNIFORM_BUFFER, BLOCK_F, buf, 0 * stride, sizeof fdata);
  GLCHECK;
  glBindBufferRange (GL_UNIFORM_BUFFER, BLOCK_G, buf, 1 * stride, sizeof gdata);
  GLCHECK;

  // The remaining uniform block, "H", contains per-object attributes.

  // Leave the "H" uniform buffer bound to GL_UNIFORM_BUFFER (but not to a
  // particular index), so we don't have to bind before we update the uniform
  // buffer in uniform_buffer_t::update().
  uniform_buffer.bind ();

  // Match the ClearColor to the fog background.
  glClearColor (fdata.b [0], fdata.b [1], fdata.b [2], 0.0f); GLCHECK;
}

bool program_t::initialize ()
{
  if (! uniform_buffer.initialize ()) return false;

  GLint status = 0;
  GLuint vshader = make_shader (GL_VERTEX_SHADER, IDR_VERTEX_SHADER);
  GLuint gshader = make_shader (GL_GEOMETRY_SHADER, IDR_GEOMETRY_SHADER);
  GLuint fshader = make_shader (GL_FRAGMENT_SHADER, IDR_FRAGMENT_SHADER);
  if (vshader && gshader && fshader) {
    id = glCreateProgram (); GLCHECK;
    if (id) {
      glAttachShader (id, vshader); GLCHECK;
      glAttachShader (id, gshader); GLCHECK;
      glAttachShader (id, fshader); GLCHECK;
      glLinkProgram (id); GLCHECK;
      glGetProgramiv (id, GL_LINK_STATUS, & status); GLCHECK;
#if PRINT_ENABLED
      std::cout << "Shader program linking "
                << (status ? "succeeded." : "failed.") << std::endl;
      PRINT_INFO_LOG (id, glGetProgramiv, glGetProgramInfoLog);
      GLCHECK;
#endif
    }
    glDetachShader (id, fshader); GLCHECK;
    glDetachShader (id, gshader); GLCHECK;
    glDetachShader (id, vshader); GLCHECK;
    glDeleteShader (fshader); GLCHECK;
    glDeleteShader (gshader); GLCHECK;
    glDeleteShader (vshader); GLCHECK;
  }

  if (status) {
    glUseProgram (id); GLCHECK;
  }

  return status;
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
  glBindBufferRange (GL_UNIFORM_BUFFER, BLOCK_H, uniform_buffer_id,
                     uniform_buffer_offset, block_size);
  GLCHECK;
  glCullFace (GL_FRONT); GLCHECK;
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
  GLCHECK;
  glCullFace (GL_BACK); GLCHECK;
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
  GLCHECK;
}
