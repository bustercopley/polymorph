// -*- C++ -*-
#version 420
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 p;
uniform mat4 m;
uniform vec3 g;
uniform vec3 l;
uniform vec3 s;
uniform vec3 d;
uniform float r;
uniform float f;
uniform float e;

in vec3 S [6];
out flat vec3 C;
out noperspective vec3 E;

float sq (float x) { return x * x; }

vec3 color (vec3 x)
{
  vec3 n = (m * vec4 (x, 0)).xyz;
  vec3 p = (m * vec4 (r * x, 1)).xyz;
  vec3 a = normalize (p - l);
  return (d * max (0, -dot (a, n)) + s * pow (max (0, -dot (normalize (p), reflect (a, n))), 10)) * f * (p [2] - e);
}

void vertex (vec4 X, vec3 c, vec3 e)
{
  C = c;
  E = e;
  gl_Position = X;
  EmitVertex ();
}
