#include "compiler.h"
#include "aligned-arrays.h"
#include "config.h"
#include "mswin.h"
#include "vector.h"
#include "glinit.h"
#include "graphics.h"
#include "model.h"
#include <cstdint>

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
    text [0] = get_resource_data (id1, & size [0]);
    text [1] = get_resource_data (id2, & size [1]);

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

bool initialize_programs (program_t (& programs) [2], v4f view)
{
  glEnable (GL_DEPTH_TEST);
  glDepthRange (1.0, 0.0);
  glEnable (GL_CULL_FACE);
  //glCullFace (GL_BACK);
  glEnable (GL_BLEND);
  glBlendEquation (GL_FUNC_ADD);
  glBlendFunc (GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return
    programs [0].initialize (view, 260) &&
    programs [1].initialize (view, 261);
}

bool program_t::initialize (v4f view, unsigned gshader2)
{
  id = glCreateProgram ();
  GLuint vshader_id = make_shader (GL_VERTEX_SHADER, 258, 0);
  GLuint gshader_id = make_shader (GL_GEOMETRY_SHADER, 259, gshader2);
  GLuint fshader_id = make_shader (GL_FRAGMENT_SHADER, 262, 0);
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
    const char * names = "p\0l\0g\0m\0r\0d\0s\0f\0e\0";
    uniform_locations [k] = glGetUniformLocation (id, names + 2 * k);
  }

  float t [4] ALIGNED16;
  store4f (t, view);
  float z0 = - t [0];
  float z1 = - t [0] - t [1];
  float zd =  - t [1];
  float w = t [2];
  float h = t [3];

  float projection_matrix [16] = {
   -2*z0/w,   0,       0,           0,
    0,       -2*z0/h,  0,           0,
    0,        0,      (z0+z1)/zd,  -1,
    0,        0,     -2*z0*z1/zd,   0,
  };

  float light_position [] [3] = {
    { 0.0f, 0.0f, 2 * z0 / 3, },
  };

  glUseProgram (id);
  glUniformMatrix4fv (uniform_locations [uniforms::p], 1, GL_FALSE, projection_matrix);
  glUniform3fv (uniform_locations [uniforms::l], 1, & light_position [0] [0]);
  glUniform3fv (uniform_locations [uniforms::s], 1, usr::specular_material);
  glUniform1f (uniform_locations [uniforms::f], -1.0f / zd);
  glUniform1f (uniform_locations [uniforms::e], z1);
  //glUseProgram (0);

  return true;
}

void clear ()
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void paint (float r,
            const float (& m) [16],
            const float (& g) [4],
            const float (& d) [4],
            unsigned N,
            unsigned vao_id,
            const program_t & program)
{
  glUseProgram (program.id);
  glUniform1f (program.uniform_locations [uniforms::r], r);
  glUniformMatrix4fv (program.uniform_locations [uniforms::m], 1, GL_FALSE, m);
  glUniform3fv (program.uniform_locations [uniforms::g], 1, g);
  glUniform3fv (program.uniform_locations [uniforms::d], 1, d);
  glBindVertexArray (vao_id);
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
  glBindVertexArray (0);
  //glUseProgram (0);
}
