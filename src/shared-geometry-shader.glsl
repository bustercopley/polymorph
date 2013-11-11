// -*- C++ -*-
#version 420
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 p;
uniform vec3 l;
uniform float f [2];

layout (std140) uniform H {
  mat4 m;
  vec4 g;
  vec4 h [3];
  vec4 d;
  float r;
};

in vec3 S [6];
out flat vec3 N;
out noperspective vec3 E;

void color (vec3 x)
{
  vec3 n = (m * vec4 (x, 0)).xyz;
  vec3 p = (m * vec4 (r * x, 1)).xyz;
  vec3 a = normalize (p - l);
  N = (d.xyz * max (0, -dot (a, n)) + 0.3 * pow (max (0, -dot (normalize (p), reflect (a, n))), 10)) * f [1] * (p [2] - f [0]);                                                                                                             ;
}

void vertex (vec4 X, vec3 e)
{
  E = e;
  gl_Position = X;
  EmitVertex ();
}
