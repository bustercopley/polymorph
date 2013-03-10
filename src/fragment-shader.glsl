// -*- C++ -*-
#version 420

in flat vec3 C;
in noperspective vec3 E;
out vec4 c;

float amplify (float d)
{
  d = clamp (2 - 100 * d, 0, 1);
  return (4./3) * (exp2 (-2 * d * d) - 0.25);
}

void main ()
{
  float d = min (min (E [0], E [1]), E [2]);
  c = vec4 (amplify (d) * C, 1.0);
}
