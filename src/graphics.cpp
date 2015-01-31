#include "graphics.h"
#include "compiler.h"
#include "memory.h"
#include "resources.h"
#include "glinit.h"
#include "print.h"

#include <cstddef>
#include <cstdint>

namespace
{
#if PRINT_ENABLED
#define PRINT_INFO_LOG(a, b, c) print_info_log (a, b, c)
  inline void print_info_log (GLuint object,
                              PFNGLGETSHADERIVPROC glGet__iv,
                              PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
  {
    if (PRINT_ENABLED) {
      GLint log_length;
      glGet__iv (object, GL_INFO_LOG_LENGTH, & log_length);
      char * log = (char *) allocate_internal (log_length + 1);
      if (log) {
        glGet__InfoLog (object, log_length, NULL, log);
        log [log_length] = '\0';
        if (log [0]) print (log);
        deallocate (log);
      }
    }
  }
#else
#define PRINT_INFO_LOG(a, b, c)
#endif

  GLuint make_shader (GLenum type, int resource_id)
  {
    const char * text;
    GLint size;
    get_resource_data (resource_id, text, size);

    GLint id = glCreateShader (type);
    glShaderSource (id, 1, & text, & size);
    glCompileShader (id);
    GLint status = 0;
    glGetShaderiv (id, GL_COMPILE_STATUS, & status);
#if PRINT_ENABLED
    switch (resource_id) {
    case IDR_VERTEX_SHADER: std::cout << "Vertex "; break;
    case IDR_FRAGMENT_SHADER: std::cout << "Fragment "; break;
    case IDR_GEOMETRY_SHADER: std::cout << "Geometry "; break;
    default: ;
    }
    std::cout << "Shader compilation " << (status ? "succeeded.\n" : "failed.\n");
    PRINT_INFO_LOG (id, glGetShaderiv, glGetShaderInfoLog);
#endif
    return status ? id : 0;
  }
}

static const int attribute_id_x = 0;

unsigned make_vao (unsigned N, const float (* vertices) [4], const unsigned (* indices) [6])
{
  unsigned vao_id;
  glGenVertexArrays (1, & vao_id);
  glBindVertexArray (vao_id);
  GLuint buffer_ids [2];
  glGenBuffers (2, buffer_ids);
  glBindBuffer (GL_ARRAY_BUFFER, buffer_ids [0]);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, buffer_ids [1]);
  glBufferData (GL_ARRAY_BUFFER, (N + 2) * 4 * sizeof (float), vertices, GL_STATIC_DRAW);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * N * sizeof (unsigned), indices, GL_STATIC_DRAW);
  glVertexAttribPointer (attribute_id_x, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray (attribute_id_x);
  return vao_id;
}

namespace
{
  // Align x upwards to given alignment (which must be a power of 2). Store the result in y.
  template <typename Dest, typename Source>
  inline void align_up (Dest & y, Source x, std::intptr_t alignment)
  {
    y = (Dest) ((((std::intptr_t) (x)) + (alignment - 1)) & - alignment);
  }
}

uniform_buffer_t::~uniform_buffer_t ()
{
  deallocate (m_memory);
}

bool uniform_buffer_t::initialize ()
{
  GLint size, align;
  glGetIntegerv (GL_MAX_UNIFORM_BLOCK_SIZE, & size);
  glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, & align);
  // Align client-side buffer to at least 64 bytes, to avoid straddling cache lines.
  align = align > 64 ? align : 64;

  m_size = size;
  m_memory = allocate_internal (m_size + align);
  if (! m_memory) return false;
  align_up (m_begin, m_memory, align);
  align_up (m_stride, sizeof (uniform_block_t), align);

  glGenBuffers (1, & m_id);
  glBindBuffer (GL_UNIFORM_BUFFER, m_id);

  return true;
}

void uniform_buffer_t::update ()
{
  // Create a buffer, orphaning any previous buffer.
  glBufferData (GL_UNIFORM_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW);
  // Upload the buffer data.
  glBufferSubData (GL_UNIFORM_BUFFER, 0, m_size, m_begin);
}

bool initialize_program (program_t & program)
{
  glEnable (GL_DEPTH_CLAMP);
  glEnable (GL_CULL_FACE);
  glEnable (GL_BLEND);
  glBlendFunc (GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return program.initialize ();
}

void set_view (const float (& view) [4], int width, int height, GLuint * uniform_locations)
{
  glViewport (0, 0, width, height);
  float z1 = view [0];
  float z2 = view [1];
  float x1 = view [2];
  float y1 = view [3];
  float zd = z2-z1;

  float projection_matrix [16] = {
   -z1/x1,    0,       0,           0,
    0,       -z1/y1,   0,           0,
    0,        0,      (z1+z2)/zd,  -1,
    0,        0,     -2*z1*z2/zd,   0,
  };

  float light_position [] [3] = {
    { 0.0f, 0.0f, 2 * z1 / 3, },
  };

  GLsizei light_count = sizeof light_position / sizeof * light_position;

  float fog_coefficients [] = { z2, -1.0f / zd, };

  // Values in the default uniform block.
  glUniformMatrix4fv (uniform_locations [uniforms::p], 1, GL_FALSE, projection_matrix);
  glUniform3fv (uniform_locations [uniforms::l], light_count, & light_position [0] [0]);
  glUniform1fv (uniform_locations [uniforms::f], 2, fog_coefficients);
}

bool program_t::initialize ()
{
  GLint status = 0;
  GLuint vshader_id = make_shader (GL_VERTEX_SHADER, IDR_VERTEX_SHADER);
  GLuint gshader_id = make_shader (GL_GEOMETRY_SHADER, IDR_GEOMETRY_SHADER);
  GLuint fshader_id = make_shader (GL_FRAGMENT_SHADER, IDR_FRAGMENT_SHADER);
  if (vshader_id && gshader_id && fshader_id) {
    id = glCreateProgram ();
    if (id) {
      glAttachShader (id, vshader_id);
      glAttachShader (id, gshader_id);
      glAttachShader (id, fshader_id);
      glBindAttribLocation (id, attribute_id_x, "x");
      glLinkProgram (id);
      glGetProgramiv (id, GL_LINK_STATUS, & status);
#if PRINT_ENABLED
      std::cout << "Shader program linking " << (status ? "succeeded.\n" : "failed.\n");
      PRINT_INFO_LOG (id, glGetProgramiv, glGetProgramInfoLog);
#endif
    }
  }
  if (vshader_id) glDeleteShader (vshader_id);
  if (gshader_id) glDeleteShader (gshader_id);
  if (fshader_id) glDeleteShader (fshader_id);

  if (! status) {
    return false;
  }

  for (unsigned k = 0; k != uniforms::count; ++ k) {
    const char * names = "p\0l\0f\0";
    uniform_locations [k] = glGetUniformLocation (id, names + 2 * k);
  }

  // Binding for Uniform block "H".
  GLuint uniform_block_index = glGetUniformBlockIndex (id, "H");
  glUniformBlockBinding (id, uniform_block_index, 0);
  glUseProgram (id);

  return true;
}

void clear ()
{
  glClear (GL_COLOR_BUFFER_BIT);
}

void paint (unsigned N,
            unsigned vao_id,
            GLuint uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset)
{
  const unsigned size = sizeof (uniform_block_t);
  glBindVertexArray (vao_id);
  glBindBufferRange (GL_UNIFORM_BUFFER, 0, uniform_buffer_id, uniform_buffer_offset, size);
  //glCullFace (GL_FRONT);
  //glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_INT, nullptr);
  glCullFace (GL_BACK);
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_INT, nullptr);
}
