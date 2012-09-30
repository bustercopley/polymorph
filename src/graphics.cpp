#include "config.h"
#include "glprocs.h"
#include "graphics.h"
#include "model.h"
#include "systems.h"
#include "vector.h"
#include "compiler.h"
#include "aligned-arrays.h"
#include <cstdint>

#define ENABLE_PRINT
#include "print.h"

const char * uniforms::names [uniforms::count] = {
  "p", "l", "g", "m", "r", "a", "d", "s", "fogm", "fogd"
};

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

unsigned data_to_vao_id (unsigned N, float (* vertices) [4], std::uint8_t (* indices) [6])
{
  unsigned vao_id;
  glGenVertexArrays (1, & vao_id);
  glBindVertexArray (vao_id);
  GLuint buffer_ids [2];
  glGenBuffers (2, buffer_ids);
  glBindBuffer (GL_ARRAY_BUFFER, buffer_ids [0]);
  glBufferData (GL_ARRAY_BUFFER, (N + 2) * 4 * sizeof (float), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer (0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, buffer_ids [1]);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * N * sizeof (std::uint8_t), indices, GL_STATIC_DRAW);
  glBindVertexArray (0);
  return vao_id;
}

bool initialize_programs (program_t (& programs) [3], v4f view)
{
  return
    programs [0].initialize (view, 260) &&
    programs [1].initialize (view, 261) &&
    programs [2].initialize (view, 262);
}

bool program_t::initialize (v4f view, unsigned gshader2)
{
  id = glCreateProgram ();
  GLuint vshader_id = make_shader (GL_VERTEX_SHADER, 258, 0);
  GLuint gshader_id = make_shader (GL_GEOMETRY_SHADER, 259, gshader2);
  GLuint fshader_id = make_shader (GL_FRAGMENT_SHADER, 263, 0);
  if (vshader_id == 0 || gshader_id == 0 || fshader_id == 0) {
    return false;
  }

  glAttachShader (id, vshader_id);
  glAttachShader (id, gshader_id);
  glAttachShader (id, fshader_id);
  glBindAttribLocation (id, 0, "x");
  glLinkProgram (id);

  GLint status;
  glGetProgramiv (id, GL_LINK_STATUS, & status);
  if (! status) {
    print_info_log (id, glGetProgramiv, glGetProgramInfoLog);
    return false;
  }

  for (unsigned k = 0; k != uniforms::count; ++ k) {
    uniform_locations [k] = glGetUniformLocation (id, uniforms::names [k]);
  }

  glDisable (GL_DEPTH_TEST);
  glEnable (GL_CULL_FACE);
  glCullFace (GL_BACK);

  float t [4];
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
    0,        0,      -2*z0*z1/zd,  0,
  };

  float light_position [] [3] = {
    { 0.0f, 0.0f, 2 * z0 / 3, },
  };

  glUseProgram (id);
  glUniformMatrix4fv (uniform_locations [uniforms::p], 1, GL_FALSE, projection_matrix);
  glUniform3fv (uniform_locations [uniforms::l], 1, & light_position [0] [0]);
  glUniform3fv (uniform_locations [uniforms::s], 1, usr::specular_material);
  glUniform3fv (uniform_locations [uniforms::a], 1, usr::ambient_material);
  glUniform1f (uniform_locations [uniforms::fogm], -1.0f / zd);
  glUniform1f (uniform_locations [uniforms::fogd], z1);

  glUseProgram (0);

  return true;
}

void clear ()
{
  glClear (GL_COLOR_BUFFER_BIT);
}

void paint (float radius,
            const float (& modelview_matrix) [16],
            const float (& rgb0) [4],
            const float (& abc0) [4],
            unsigned N,
            unsigned vao_id,
            const program_t & program)
{
  glUseProgram (program.id);
  glUniformMatrix4fv (program.uniform_locations [uniforms::m], 1, GL_FALSE, modelview_matrix);
  glUniform1f (program.uniform_locations [uniforms::r], radius);
  glUniform3fv (program.uniform_locations [uniforms::g], 1, abc0);
  glUniform3fv (program.uniform_locations [uniforms::d], 1, rgb0);
  glBindVertexArray (vao_id);
  glDrawElements (GL_TRIANGLES_ADJACENCY, N * 6, GL_UNSIGNED_BYTE, nullptr);
  glBindVertexArray (0);
  glUseProgram (0);
}
