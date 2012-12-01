// -*- C++ -*-
#version 420

in flat vec3 gc;
in smooth vec3 ed;
out vec4 c;

float amplify (float d)
{
  d = -1.0 + 120 * d;
  d = clamp (d, 0, 1);
  d = (4.0/3) * (1 - exp2 (-2 * d * d));
  return d;
}

void main ()
{
  float d = min (min (ed [0], ed [1]), ed [2]);
  c = vec4 (amplify (d) * gc, 0.85);
}
