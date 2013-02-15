// -*- C++ -*-
#version 420

in flat vec3 gc;
in smooth vec3 ed;
out vec4 c;

float amplify (float d)
{
  d = clamp (2 - 120 * d, 0, 1);
  d = (4./3) * (exp2 (-2 * d * d) - 0.25);
  return d;
}

void main ()
{
  float d = min (min (ed [0], ed [1]), ed [2]);
  c = vec4 (amplify (d) * gc, 1.0);
}
