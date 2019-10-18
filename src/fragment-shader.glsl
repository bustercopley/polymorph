// Copyright 2012-2019 Richard Copley
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

// Scene uniforms.
layout (std140, binding = 1) uniform F
{
  vec3 a;      // ambient reflection (rgb)
  vec3 b;      // background (rgb)
  vec4 c;      // line colour (rgba)
  vec4 r;      // xyz: specular reflection (rgb), w: exponent
  vec4 f;      // coefficients for line width and fog
  vec3 l [4];  // light positions
};

// Per-object uniforms.
layout (std140, binding = 0) uniform H
{
  vec4 d;  // diffuse reflection
  vec4 g;  // generator coefficients
  mat4 m;  // modelview matrix
  bool s;  // snub?
};

in vec3 X;
in flat vec3 N;
sample in noperspective float R, S, T, V, W;

layout (location = 0) out vec4 o;

void main ()
{
  vec3 G = gl_FrontFacing ? N : -N;
  vec3 U = normalize (X);
  vec3 C = a - b;
  // Add diffuse and specular reflection from each light.
  for (int i = 0; i != 4; ++ i) {
    vec3 L = normalize (l [i] - X);
    float E = max (0, dot (L, G));
    C += d.xyz * E + pow (max (0, dot (L - 2 * E * G, U)), r.w) * r.xyz;
  }
  // Edge shading, see model.cpp.
  float e = clamp(f.x * min (R, min (min (S, T), min (V, W))) + f.y, 0, 1);
  o = c + e * (vec4 ((f.z + f.w * X.z) * C + b, d.w) - c);
}
