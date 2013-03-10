// -*- C++ -*-
#version 420

in vec4 x;
out vec3 S;

void main()
{
  S = x.xyz;
}
