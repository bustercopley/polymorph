// -*- C++ -*-
#version 420
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 12) out;

uniform mat4 p;
uniform mat4 m;
uniform vec3 g;
uniform vec3 l;
uniform vec3 a;
uniform vec3 d;
uniform vec3 s;
uniform float r;
uniform float fogm;
uniform float fogd;

in vec3 xs [6];
out vec4 gc;
out vec3 ed;

vec4 color (vec3 x)
{
  vec3 normal = (m * vec4 (x, 0.0)).xyz;
  vec3 position = (m * vec4 (r * x, 1.0)).xyz;
  vec3 ld = normalize (position - l);
  vec3 dc = d * max (0.0, -dot (ld, normal));
  vec3 sc = s * pow (max (0.0, -dot (normalize (position), reflect (ld, normal))), 40);
  return vec4 (a + (dc + sc) * fogm * (position [2] - fogd), 0.0);
}

void segment (vec4 A, vec4 V, vec4 W, vec4 c)
{
  gc = c;
  ed = vec3 (1.0, 1.0, 1.0);
  gl_Position = A;
  EmitVertex ();

  gc = c;
  ed = vec3 (0.0, 1.0, 1.0);
  gl_Position = V;
  EmitVertex ();

  gc = c;
  ed = vec3 (0.0, 1.0, 1.0);
  gl_Position = W;
  EmitVertex ();

  EndPrimitive ();
}

void triangle (vec4 U, vec4 V, vec4 W, vec4 c)
{
  gc = c;
  ed = vec3 (3.0, 0.0, 0.0);
  gl_Position = W;
  EmitVertex ();

  gc = c;
  ed = vec3 (0.0, 3.0, 0.0);
  gl_Position = V;
  EmitVertex ();

  gc = c;
  ed = vec3 (0.0, 0.0, 3.0);
  gl_Position = U;
  EmitVertex ();

  EndPrimitive ();
}
