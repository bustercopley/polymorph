#include "graphics.h"
#include "compiler.h"
#include "memory.h"
#include "resources.h"
#include "glinit.h"
#include "print.h"

namespace
{
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
        print (log);
        deallocate (log);
      }
    }
  }

  GLuint make_shader (GLenum type, int id1, int id2)
  {
    const char * text [2];
    GLint size [2];
    get_resource_data (id1, text [0], size [0]);
    get_resource_data (id2, text [1], size [1]);

    GLint id = glCreateShader (type);
    glShaderSource (id, 2, const_cast <const char **> (text), size);
    glCompileShader (id);
    GLint status = 0;
    glGetShaderiv (id, GL_COMPILE_STATUS, & status);
    if (! status) {
      print ("Shader compilation failed: ", 1000 * id1 + id2);
      print_info_log (id, glGetShaderiv, glGetShaderInfoLog);
      return 0;
    }
    return id;
  }
}

static const int attribute_id = 0;

unsigned make_vao (unsigned N, const float (* vertices) [4], const std::uint8_t (* indices) [6])
{
  unsigned vao_id;
  glGenVertexArrays (1, & vao_id);
  glBindVertexArray (vao_id);
  GLuint buffer_ids [2];
  glGenBuffers (2, buffer_ids);
  glBindBuffer (GL_ARRAY_BUFFER, buffer_ids [0]);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, buffer_ids [1]);
  glBufferData (GL_ARRAY_BUFFER, (N + 2) * 4 * sizeof (float), vertices, GL_STATIC_DRAW);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * N * sizeof (std::uint8_t), indices, GL_STATIC_DRAW);
  glVertexAttribPointer (attribute_id, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray (attribute_id);
  //glBindVertexArray (0);
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

bool initialize_programs (program_t (& programs) [2])
{
  glEnable (GL_DEPTH_TEST);
  glDepthRange (1.0, 0.0);
  glEnable (GL_CULL_FACE);
  //glCullFace (GL_BACK);
  glEnable (GL_BLEND);
  glBlendEquation (GL_FUNC_ADD);
  glBlendFunc (GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return
    programs [0].initialize (IDR_GEOMETRY_SHADER) &&
    programs [1].initialize (IDR_SNUB_GEOMETRY_SHADER);
}

void set_view (program_t (& programs) [2], const float (& view) [4])
{
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

  float fog_coefficients [] = { z2, -1.0f / zd, };

  for (unsigned i = 0; i != 2; ++ i)
  {
    program_t p = programs [i];
    // Values in the default uniform block.
    glUseProgram (p.id);
    glUniformMatrix4fv (p.uniform_locations [uniforms::p], 1, GL_FALSE, projection_matrix);
    glUniform3fv (p.uniform_locations [uniforms::l], 1, & light_position [0] [0]);
    glUniform1fv (p.uniform_locations [uniforms::f], 2, fog_coefficients);
  }
}

bool program_t::initialize (unsigned gshader2)
{
  id = glCreateProgram ();
  GLuint vshader_id = make_shader (GL_VERTEX_SHADER, IDR_VERTEX_SHADER, 0);
  GLuint gshader_id = make_shader (GL_GEOMETRY_SHADER, IDR_SHARED_GEOMETRY_SHADER, gshader2);
  GLuint fshader_id = make_shader (GL_FRAGMENT_SHADER, IDR_FRAGMENT_SHADER, 0);
  if (vshader_id == 0 || gshader_id == 0 || fshader_id == 0) {
    return false;
  }

  glAttachShader (id, vshader_id);
  glAttachShader (id, gshader_id);
  glAttachShader (id, fshader_id);
  glBindAttribLocation (id, attribute_id, "x");
  glLinkProgram (id);

  GLint status;
  glGetProgramiv (id, GL_LINK_STATUS, & status);
  if (! status) {
    print_info_log (id, glGetProgramiv, glGetProgramInfoLog);
    return false;
  }

  for (unsigned k = 0; k != uniforms::count; ++ k) {
    const char * names = "p\0l\0f\0";
    uniform_locations [k] = glGetUniformLocation (id, names + 2 * k);
  }

  // Binding for Uniform block "H".
  GLuint uniform_block_index = glGetUniformBlockIndex (id, "H");
  glUniformBlockBinding (id, uniform_block_index, 0);

  return true;
}

void clear ()
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void paint (unsigned N,
            unsigned vao_id,
            const program_t & program,
            std::int32_t uniform_buffer_id,
            std::ptrdiff_t uniform_buffer_offset)
{
  const unsigned size = sizeof (uniform_block_t);
  glUseProgram (program.id);
  glBindVertexArray (vao_id);
  glBindBufferRange (GL_UNIFORM_BUFFER, 0, uniform_buffer_id, uniform_buffer_offset, size);
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
  //glBindVertexArray (0);
  //glUseProgram (0);
}
