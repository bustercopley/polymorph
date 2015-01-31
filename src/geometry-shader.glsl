// -*- C++ -*-

#version 420

layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 p;
uniform vec3 l;
uniform float f [2];

layout (std140) uniform H {
  mat4 m;
  vec4 g;
  vec4 h [3];
  vec4 d;
  float r;
  bool s;
};

in vec3 S [6];
out flat vec4 N;
out noperspective vec3 E;

void color (vec3 x)
{
  vec3 n = (m * vec4 (x, 0)).xyz;
  vec3 p = (m * vec4 (r * x, 1)).xyz;
  vec3 a = normalize (p - l);
  N = vec4 ((d.xyz * max (0, -dot (a, n)) + 0.3 * pow (max (0, -dot (normalize (p), reflect (a, n))), 10)) * f [1] * (p [2] - f [0]), d.w);
}

void vertex (vec4 X, vec3 e)
{
  E = e;
  gl_Position = X;
  EmitVertex ();
}

void segment (vec4 A, vec4 V, vec4 W, float h, float k, float l)
{
  vertex (A, vec3 (h, k, k));
  vertex (V, vec3 (0, 0, l));
  vertex (W, vec3 (0, l, 0));
  EndPrimitive ();
}

void aspect (vec4 A, vec4 T, vec4 V, vec4 W, vec3 S, vec4 h)
{
  color (S);
  segment (A, V, T, h [0], h [1], h [2]);
  segment (A, T, W, h [1], h [0], h [3]);
}

void segment (vec3 S, vec4 A, vec4 V, vec4 W, float h)
{
  color (S);
  vertex (A, vec3 (h));
  vertex (V, vec3 (0));
  vertex (W, vec3 (0));
  EndPrimitive ();
}

void main ()
{
  vec3 G = r * g.xyz;
  vec3 x = G [0] * S [0];
  vec3 Y = G [1] * S [1];
  vec3 z = G [2] * S [2];
  vec3 X = G [0] * S [3];
  vec3 y = G [1] * S [4];
  vec3 Z = G [2] * S [5];

  vec3 u = X + y + z;
  vec3 v = x + Y + z;
  vec3 w = x + y + Z;

  mat4 q = p * m;

  vec4 U = q * vec4 (u, 1);
  vec4 V = q * vec4 (v, 1);
  vec4 W = q * vec4 (w, 1);

  if (s) {
    vec3 P = w - v;
    vec3 Q = u - w;
    vec3 R = v - u;

    float A = dot (P, P);
    float B = dot (Q, Q);
    float C = dot (R, R);

    float D = dot (Q, R);
    float E = dot (R, P);
    float F = dot (P, Q);

    float s = dot (S [4], u);
    float t = dot (S [2], u);

    color (normalize (cross (v - u, u - w)));
    vertex (W, vec3 (0, 0, sqrt (B - D * D / C)));
    vertex (V, vec3 (0, sqrt (A - F * F / B), 0));
    vertex (U, vec3 (sqrt (C - E * E / A), 0, 0));
    EndPrimitive ();

    segment (S [4], q * vec4 (s * S [4], 1), W, U, sqrt (r * r - s * s - B / 4));
    segment (S [2], q * vec4 (t * S [2], 1), U, V, sqrt (r * r - t * t - C / 4));
  }
  else {
    vec3 t = x + y + z;
    mat4 q = p * m;
    vec4 T = q * vec4 (t, 1);
    aspect (q * vec4 (dot (S [0], t) * S [0], 1), T, V, W, S [0], r * h [0]);
    aspect (q * vec4 (dot (S [4], t) * S [4], 1), T, W, U, S [4], r * h [1]);
    aspect (q * vec4 (dot (S [2], t) * S [2], 1), T, U, V, S [2], r * h [2]);
  }
}
