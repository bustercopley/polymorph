// -*- C++ -*-

#version 430

layout (std140) uniform F
{
  vec3 l [4];
  vec3 a;
  vec3 b;
  vec4 c;
  vec4 r;
  vec4 f;
};

layout (std140) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

in vec3 X;
in flat vec3 N;
in noperspective float E [5];

out vec4 o;

void main ()
{
  vec3 H = gl_FrontFacing ? N : -N;
  vec3 C = a + b;
  vec3 U = normalize (X);
  for (int i = 0; i != 4; ++ i) {
    vec3 L = normalize (l [i] - X);
    float F = max (0, dot (L, H));
    C += d.xyz * F + pow (max (0, dot (L - 2 * F * H, U)), r.w) * r.xyz;
  }
  float e = clamp (f.x + f.y * min (min (min (E [0], E [1]), min (E [2], E [3])), E [4]), 0, 1);
  o = c + 1.33 * (1 - exp2 (-2 * e * e)) * (vec4 (b + (f.z + f.w * X.z) * (C - b), d.w) - c);
}
