#include "graphics.h"
#include "compiler.h"
#include "memory.h"
#include "resources.h"
#include "glinit.h"
#include "vector.h"
#include "print.h"

#include <cstddef>
#include <cstdint>

#define OBJECT_UNIFORM_BINDING_INDEX 0

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
  //glDeleteBuffers (1, & m_id);
  deallocate (m_memory);
}

bool uniform_buffer_t::initialize ()
{
  GLint max_size, align;
  glGetIntegerv (GL_MAX_UNIFORM_BLOCK_SIZE, & max_size);
  glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, & align);
  // Align client-side buffer to at least 64 bytes, to avoid straddling cache lines.
  align = align > 64 ? align : 64;
  m_size = max_size;
  m_memory = allocate_internal (m_size + align);
  if (! m_memory) return false;
  align_up (m_begin, m_memory, align);
  align_up (m_stride, sizeof (object_data_t), align);
  glGenBuffers (1, & m_id);
  return true;
}

void uniform_buffer_t::bind ()
{
  glBindBuffer (GL_UNIFORM_BUFFER, m_id);
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
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return program.initialize ();
}

void program_t::set_view (const float (& view) [4],
                          float width, float height,
                          const float (& colours) [4] [4],
                          float fog_near, float fog_far,
                          float line_width_extra, float line_sharpness)
{
  glViewport (0, 0, width, height);

  const float (& ambient) [4] = colours [0];
  const float (& background) [4] = colours [1];
  const float (& line_colour) [4] = colours [2];
  const float (& specular) [4] = colours [3];

  glClearColor (background [0], background [1], background [2], 0.0f);

  float z1 = view [0];  // z coord of screen (front of tank) (a negative number)
  float z2 = view [1];  // z coord of back of tank (a negative number) (|z2| > |z1|)
  float x1 = view [2];  // x coord of right edge of screen (and of front-right edge of tank)
  float y1 = view [3];  // y coord of top edge of screen (and of front-top edge of tank)
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
    GLfloat l [4] [4];  // vec3 [4],  light positions
    GLfloat a [4];      // vec3,      ambient reflection (rgb)
    GLfloat b [4];      // vec3,      background (rgb)
    GLfloat c [4];      // vec4,      line colour (rgba)
    GLfloat r [4];      // vec4,      xyz: specular reflection (rgb); w: exponent
    GLfloat f [4];      // vec4,      coefficients for line width and fog
  };

  struct geometry_data_t
  {
    GLfloat p [4] [4];  // mat4,      projection matrix
    GLfloat q [4];      // vec3,      pixel scale of normalized device coordinates
  };

  ALIGNED16 fragment_data_t fragment_data = {
    {
      { -0.6f * x1, -0.2f * y1, z1 + y1, 0.0f, },
      { -0.2f * x1, +0.6f * y1, z1 + x1, 0.0f, },
      { +0.2f * x1, -0.6f * y1, z1 + y1, 0.0f, },
      { +0.6f * x1, +0.2f * y1, z1 + x1, 0.0f, },
    },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { 0, 0, 0, 0, },
    { line0, line1, fog0, fog1, }
  };

  store4f (fragment_data.a, load4f (ambient));
  store4f (fragment_data.b, load4f (background));
  store4f (fragment_data.c, load4f (line_colour));
  store4f (fragment_data.r, load4f (specular));

  ALIGNED16 geometry_data_t geometry_data = {
    {
      { -z1/x1,     0,       0,          0, },
      {    0,    -z1/y1,     0,          0, },
      {    0,       0,    -(z1+z2)/zd,  -1, },
      {    0,       0,     2*z1*z2/zd,   0, },
    },
    { width / 2.0f, height / 2.0f, 0, 0, },
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

  GLint align;
  glGetIntegerv (GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, & align);

  GLuint buffer_size, buffer_stride;
  align_up (buffer_stride, max_block_size, align);
  buffer_size = block_count * buffer_stride;

  GLuint buf_id;
  glGenBuffers (1, & buf_id);
  glBindBuffer (GL_UNIFORM_BUFFER, buf_id);
  glBufferData (GL_UNIFORM_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);

  // Copy each data block to a buffer range and bind the range to the named shader uniform block.

  for (std::size_t n = 0; n != block_count; ++ n) {
    glBufferSubData (GL_UNIFORM_BUFFER, n * buffer_stride, blocks [n].size, blocks [n].data);
    GLuint block_index = glGetUniformBlockIndex (id, blocks [n].name);
    GLuint binding_index = n + 1;
    glUniformBlockBinding (id, block_index, binding_index);
    glBindBufferRange (GL_UNIFORM_BUFFER, binding_index, buf_id, n * buffer_stride, blocks [n].size);
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

  // Binding for Uniform block "H".
  GLuint object_uniform_block_index = glGetUniformBlockIndex (id, "H");
  glUniformBlockBinding (id, object_uniform_block_index, OBJECT_UNIFORM_BINDING_INDEX);
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
  const unsigned block_size = sizeof (object_data_t);
  glBindVertexArray (vao_id);
  glBindBufferRange (GL_UNIFORM_BUFFER, OBJECT_UNIFORM_BINDING_INDEX, uniform_buffer_id, uniform_buffer_offset, block_size);
  glCullFace (GL_FRONT);
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
  glCullFace (GL_BACK);
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
}
