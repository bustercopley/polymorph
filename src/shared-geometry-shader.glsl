// -*- C++ -*-
#version 420
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 12) out;

uniform mat4 p;
uniform mat4 m;
uniform vec3 g;
uniform vec3 l;
uniform vec4 d;
uniform vec3 s;
uniform float r;
uniform float fogm;
uniform float fogd;

in vec3 xs [6];
out flat vec3 gc;
out vec3 ed;

vec3 color (vec3 x)
{
  vec3 normal = (m * vec4 (x, 0.0)).xyz;
  vec3 position = (m * vec4 (r * x, 1.0)).xyz;
  vec3 ld = normalize (position - l);
  vec3 dc = vec3 (d) * max (0.0, -dot (ld, normal));
  vec3 sc = s * pow (max (0.0, -dot (normalize (position), reflect (ld, normal))), 10);
  return (dc + sc) * fogm * (position [2] - fogd);
}

void vertex (vec4 X, vec3 c, vec3 e)
{
  gc = c;
  ed = e;
  gl_Position = X;
  EmitVertex ();
}

void segment (vec4 A, vec4 V, vec4 W, vec3 c)
{
  vertex (A, c, vec3 (1.0, 1.0, 1.0));
  vertex (V, c, vec3 (0.0, 1.0, 1.0));
  vertex (W, c, vec3 (0.0, 1.0, 1.0));
  EndPrimitive ();
}

void segment (vec4 T, vec4 A, vec4 V, vec4 W, vec3 c)
{
  vertex (V, c, vec3 (0.0, 1.0, 1.0));
  vertex (T, c, vec3 (0.0, 1.0, 1.0));
  vertex (A, c, vec3 (1.0, 1.0, 1.0));
  vertex (W, c, vec3 (0.0, 1.0, 1.0));
  EndPrimitive ();
}

void triangle (vec4 U, vec4 V, vec4 W, vec3 c)
{
  vertex (W, c, vec3 (3.0, 0.0, 0.0));
  vertex (V, c, vec3 (0.0, 3.0, 0.0));
  vertex (U, c, vec3 (0.0, 0.0, 3.0));
  EndPrimitive ();
}
