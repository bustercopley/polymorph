// -*- C++ -*-
#version 420

in flat vec3 gc;
in vec3 ed;
out vec4 c;

float amplify (float d, float scale, float offset)
{
  d = scale * d + offset;
  d = clamp (d, 0, 1);
  d = exp2 (-2 * d * d);
  return d;
}

void main ()
{
  float d = min (min (ed [0], ed [1]), ed [2]);
  c = vec4 (amplify (d, 50, -2.5) * gc, 0.85);
}
