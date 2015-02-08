// -*- C++ -*-

#version 430

uniform vec3 l [4];
uniform vec4 f;

layout (std140) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

in noperspective float E [5];
in smooth vec3 X;
in flat vec3 N;

out vec4 c;

void main ()
{
  vec3 C = vec3 (0.02);
  for (int i = 0; i != 4; ++ i) {
    vec3 L = normalize (l [i] - X);
    float F = max (0, dot (L, N));
    C += d.xyz * F + 0.125 * pow (max (0, dot (L - 2 * F * N, normalize (X))), 32);
  }
  float e = clamp (f.x + f.y * min (min (min (E [0], E [1]), min (E [2], E [3])), E [4]), 0, 1);
  c = vec4 (1.33 * (1 - exp2 (-2 * e * e)) * (f.z + f.w * X.z) * C, d.w);
}
