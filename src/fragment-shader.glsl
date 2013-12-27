// -*- C++ -*-

#version 420

in flat vec3 N;
in noperspective vec3 E;
out vec4 c;

float amplify (float d)
{
  d = clamp (1.0 - 33 * d, 0, 1);
  return (4./3) * (exp2 (-2 * d * d) - 0.25);
}

void main ()
{
  float d = min (min (E [0], E [1]), E [2]);
  c = vec4 (amplify (d) * N, 1.0);
}
