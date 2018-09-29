// Copyright 2012-2017 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#version 430

layout (std140, binding = 1) uniform F
{
  vec3 a;
  vec3 b;
  vec4 c;
  vec4 r;
  vec4 f;
  vec3 l [4];
};

layout (std140, binding = 0) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

in vec3 X;
in flat vec3 N;
sample in noperspective float E [5];

layout (location = 0) out vec4 o;

void main ()
{
  vec3 G = gl_FrontFacing ? N : -N;
  vec3 C = a - b;
  vec3 U = normalize (X);
  for (int i = 0; i != 4; ++ i) {
    vec3 L = normalize (l [i] - X);
    float E = max (0, dot (L, G));
    C += d.xyz * E + pow (max (0, dot (L - 2 * E * G, U)), r.w) * r.xyz;
  }
  float e = clamp (f.x + f.y * min (min (min (E [0], E [1]), min (E [2], E [3])), E [4]), 0, 1);
  o = c + 1.33 * (1 - exp2 (-2 * e * e)) * (vec4 (b + (f.z + f.w * X.z) * C, d.w) - c);
}
