// -*- C++ -*-

#version 420

uniform vec3 l [4];
uniform float f [2];

layout (std140) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

in noperspective vec3 E;
in smooth vec3 X;
in flat vec3 N;
out vec4 c;

float amplify ()
{
  float d = clamp (1 - min (E [0], min (E [1], E [2])), 0, 1);
  return 1.33 * (exp2 (-2 * d * d) - .25);
}

void main ()
{
  vec3 C = vec3 (0.02);
  for (int i = 0; i != 4; ++ i) {
    vec3 L = normalize (l [i] - X);
    float F = max (0, dot (L, N));
    C += d.xyz * F + 0.125 * pow (max (0, dot (L - 2 * F * N, normalize (X))), 32);
  }
  c = vec4 (amplify () * f [1] * (X [2] - f [0]) * C, d.w);
}
