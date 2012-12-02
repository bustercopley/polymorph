// -*- C++ -*-
#version 420
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

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
out smooth vec3 ed;

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

void segment (vec4 A, vec4 V, vec4 W, vec3 c, float h)
{
  vertex (A, c, vec3 (h, 1, 1));
  vertex (V, c, vec3 (0, 1, 1));
  vertex (W, c, vec3 (0, 1, 1));
  EndPrimitive ();
}

void triangle (vec4 U, vec4 V, vec4 W, vec3 c, float hx, float hy, float hz)
{
  vertex (U, c, vec3 (hx, 0.0, 0.0));
  vertex (V, c, vec3 (0.0, hy, 0.0));
  vertex (W, c, vec3 (0.0, 0.0, hz));
  EndPrimitive ();
}
