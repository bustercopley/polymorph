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

layout (std140, binding = 0) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

layout (location = 0) in vec4 x;

out vec3 Q;

void main ()
{
  Q = (m * vec4 (x.xyz, 0)).xyz;
}
