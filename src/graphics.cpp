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
    std::cout << "Shader compilation " << (status ? "succeeded." : "failed.") << std::endl;
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

bool initialize_graphics (program_t & program)
{
  glEnable (GL_DEPTH_CLAMP);
  glEnable (GL_CULL_FACE);
  glEnable (GL_BLEND);
  glBlendFunc (GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return program.initialize ();
}

void set_view (const float (& view) [4],
               float width, float height,
               float l0, float l1,
               GLuint * uniform_locations)
{
  float z1 = view [0];  // z coord of screen (front of tank) (a negative number)
  float z2 = view [1];  // z coord of back of tank (a negative number) (|z2| > |z1|)
  float x1 = view [2];  // x coord of right edge of screen (and of front-right edge of tank)
  float y1 = view [3];  // y coord of top edge of screen (and of front-top edge of tank)
  float zd = z1 - z2;   // tank depth

  GLfloat projection_matrix [16] = {
   -z1/x1,     0,       0,          0,
      0,    -z1/y1,     0,          0,
      0,       0,    -(z1+z2)/zd,  -1,
      0,       0,     2*z1*z2/zd,   0,
  };

  GLfloat light_position [] [3] = {
    { -0.6f * x1, -0.2f * y1, z1 + y1, },
    { -0.2f * x1, +0.6f * y1, z1 + x1, },
    { +0.2f * x1, -0.6f * y1, z1 + y1, },
    { +0.6f * x1, +0.2f * y1, z1 + x1, },
  };

  GLsizei light_count = sizeof light_position / sizeof * light_position;

  GLfloat geometry_params [3] = {
    width / 2.0f,   // viewport semiwidth in pixels
    height / 2.0f,  // viewport semiheight in pixels
    0,              // not used
  };

  float fogn = 1.0f;  // Fog factor at near plane.
  float fogf = 0.2f;  // Fog factor at far plane.
  float fogd = fogn - fogf;
  float fog1 = fogd / zd;
  float fog0 = fogf - fog1 * z2;


  GLfloat fragment_params [4] = {
    l0,    // line width parameters
    l1,
    fog0,  // fog parameters
    fog1,
  };

  glViewport (0, 0, width, height);

  // Values in the default uniform block.
  glUniformMatrix4fv (uniform_locations [uniforms::p], 1, GL_FALSE, projection_matrix);
  glUniform3fv (uniform_locations [uniforms::l], light_count, & light_position [0] [0]);
  glUniform3fv (uniform_locations [uniforms::q], 1, geometry_params);
  glUniform4fv (uniform_locations [uniforms::f], 1, fragment_params);
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
      std::cout << "Shader program linking " << (status ? "succeeded." : "failed.") << std::endl;
      PRINT_INFO_LOG (id, glGetProgramiv, glGetProgramInfoLog);
#endif
      glDetachShader (id, fshader_id);
      glDetachShader (id, gshader_id);
      glDetachShader (id, vshader_id);
    }
  }
  if (fshader_id) glDeleteShader (fshader_id);
  if (gshader_id) glDeleteShader (gshader_id);
  if (vshader_id) glDeleteShader (vshader_id);

  if (! status) {
    //if (id) glDeleteProgram (id);
    return false;
  }

  for (unsigned k = 0; k != uniforms::count; ++ k) {
    const char * names = "p\0q\0l\0f\0";
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
