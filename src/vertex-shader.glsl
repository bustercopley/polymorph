// -*- C++ -*-

#version 420

layout (std140) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

in vec4 x;

out vec3 Q;

void main ()
{
  Q = (m * vec4 (x.xyz, 0)).xyz;
}
