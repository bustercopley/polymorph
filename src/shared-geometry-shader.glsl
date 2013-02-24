// -*- C++ -*-
#version 420
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 p;
uniform mat4 m;
uniform vec3 g;
uniform vec3 l;
uniform vec3 s;
uniform vec4 d;
uniform float r;
uniform float f;
uniform float e;

in vec3 S [6];
out flat vec3 C;
out smooth vec3 E;

float sq (float x) { return x * x; }

vec3 color (vec3 x)
{
  vec3 n = (m * vec4 (x, 0)).xyz;
  vec3 p = (m * vec4 (r * x, 1)).xyz;
  vec3 a = normalize (p - l);
  return (vec3 (d) * max (0, -dot (a, n)) + s * pow (max (0, -dot (normalize (p), reflect (a, n))), 10)) * f * (p [2] - e);
}

void vertex (vec4 X, vec3 c, vec3 e)
{
  C = c;
  E = e;
  gl_Position = X;
  EmitVertex ();
}

void segment (vec4 A, vec4 V, vec4 W, vec3 c, float h)
{
  vertex (A, c, vec3 (h, 1, 1));
  vertex (V, c, vec3 (0, 1, 1));
  vertex (W, c, vec3 (0, 1, 1));
  EndPrimitive ();
}

void triangle (vec4 U, vec4 V, vec4 W, vec3 u, vec3 v, vec3 w, float a, float b, float c)
{
  vec3 C = color (normalize (cross (v - u, w - u)));
  vertex (U, C, vec3 (a, 0, 0));
  vertex (V, C, vec3 (0, b, 0));
  vertex (W, C, vec3 (0, 0, c));
  EndPrimitive ();
}

void snub (vec3 s, vec3 t, vec3 x, vec3 Y, vec3 z, vec3 X, vec3 y, vec3 Z)
{
  vec3 u = X + y + z;
  vec3 v = x + Y + z;
  vec3 w = x + y + Z;
  vec3 P = w - v;
  vec3 Q = u - w;
  vec3 R = v - u;
  vec3 b = dot (s, u) * s;
  vec3 c = dot (t, u) * t;
  vec3 g = u - b;
  vec3 h = u - c;
  float A = dot (P, P);
  float B = dot (Q, Q);
  float C = dot (R, R);
  float G = 1 / B;
  float H = 1 / C;
  float D = dot (Q, R);
  float E = dot (R, P);
  float F = dot (P, Q);
  float e = dot (g, Q);
  float f = dot (h, R);
  mat4 q = p * m;
  vec4 U = q * vec4 (u, 1);
  vec4 V = q * vec4 (v, 1);
  vec4 W = q * vec4 (w, 1);
  triangle (U, W, V, u, w, v, sqrt (C - E * E / A), sqrt (B - D * D * H), sqrt (A - F * F * G));
  segment (q * vec4 (b, 1), W, U, color (s), sqrt (dot (g, g) - e * e * G));
  segment (q * vec4 (c, 1), U, V, color (t), sqrt (dot (h, h) - f * f * H));
}
