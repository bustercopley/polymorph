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

layout (std140, binding = 1) uniform F
{
  vec3 a;      // ambient reflection (rgb)
  vec3 b;      // background (rgb)
  vec4 c;      // line colour (rgba)
  vec4 r;      // xyz: specular reflection (rgb), w: exponent
  vec4 f;      // coefficients for line width and fog
  vec3 l [4];  // light positions
};

layout (std140, binding = 0) uniform H
{
  vec4 d;  // diffuse reflection
  vec4 g;  // uniform coefficients
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
  // Edge shading.     // d: distance of sample from the edge.
  //                   // e: fade factor between edge and face colours.
  // e ^               // For d1 <= d <= d2, e = x + y * d, or d = (e - x) / y.
  //   |
  // 1 +      +---     // e(d1) = 0, so d1 = -x / y.
  //   |     /         // e(d2) = 1, so d2 = (1 - x) / y.
  //   |    /          // Thus d2 - d1 = 1 / y, and given d1 and d2, we have:
  // 0 +===+--+---> d  // y = 1 / (d2 - d1), x = 1 - y * d2.
  //   0   d1 d2
  float e = clamp(f.x + f.y * min (R, min (min (S, T), min (V, W))), 0, 1);
  o = c + e * (vec4 ((f.z + f.w * X.z) * C + b, d.w) - c);
}
