// -*- C++ -*-

#version 430

layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 p;
uniform vec3 q;

layout (std140) uniform H
{
  vec4 d;
  vec4 g;
  mat4 m;
  bool s;
};

in vec3 Q [6];

out vec3 X;
out flat vec3 N;
out noperspective float E [5];

vec3 O = m [3].xyz;

void vertex (vec3 x, vec4 p, float e [5])
{
  E = e;
  X = x;
  gl_Position = p;
  EmitVertex ();
}

void triangle (vec3 A, vec3 B, vec3 C, vec4 x, vec4 y, vec4 z, float e [5], float f [5], float g [5])
{
  vertex (A, x, e);
  vertex (B, y, f);
  vertex (C, z, g);
  EndPrimitive ();
}

vec4 project (vec3 x)
{
  return p * vec4 (x, 1);
}

vec2 pdivide (vec4 s)
{
  return vec2 (q.x * s.x / s.w, q.y * s.y / s.w);
}

vec2 raster (vec3 x)
{
  return pdivide (project (x));
}

vec2 perp (vec2 a, vec2 b)
{
  return normalize (vec2 (a.y - b.y, b.x - a.x));
}

float dist (vec2 x, vec2 a, vec2 b)
{
  return dot (perp (a, b), x - b);
}

void snub_segment (vec3 Q, vec3 U, vec3 V, vec4 y, vec4 z)
{
  N = Q;
  vec3 C = O + dot (U - O, Q) * Q;
  vec3 i = U - C;
  vec3 j = V - C;
  vec4 x = project (C);
  float d = dot (i, j);
  vec2 t = raster (C - j + (2 * d / dot (i, i)) * i);
  vec2 w = raster (C - i + (2 * d / dot (j, j)) * j);
  vec2 c = pdivide (x);
  vec2 u = pdivide (y);
  vec2 v = pdivide (z);
  vec2 k = perp (t, u);
  vec2 l = perp (v, w);
  triangle (C, U, V, x, y, z,
            float [5] (dot (k, c - u), dist (c, u, v), dot (l, c - w), 1e9, 1e9),
            float [5] (0, 0, dot (l, u - w), 1e9, 1e9),
            float [5] (dot (k, v - u), 0, 0, 1e9, 1e9));
}

void aspect (vec3 Q, vec3 V, vec3 W, vec3 X, vec4 h, vec4 i, vec4 j, vec2 v, vec2 w, vec2 x)
{
  N = Q;
  vec3 A = O + dot (W - O, Q) * Q;
  vec3 k = X - A;
  vec3 l = V - A;
  vec3 d = W - 2 * A;
  vec3 e = d + V;
  vec3 f = d + X;
  vec3 E = e *  (2 / dot (e, e));
  vec3 F = f *  (2 / dot (f, f));
  vec3 U = dot (e, k) * E - k;
  vec3 Y = dot (f, l) * F - l;

  vec4 m [6];
  vec4 g = project (A);

  vec2 a = pdivide (g);
  vec2 q [7] = { raster (A + dot (e, Y) * E - Y), raster (A + U), v, w, x, raster (A + Y), raster (A + dot (f, U) * F - U) };

  for (int i = 0; i < 6; ++ i) {
    vec2 c = q [i];
    vec2 d = q [i + 1] - c;
    float l = dot (d, d);
    m [i] = l < 1e-3 ? vec4 (1e9) : inversesqrt (l) * vec2 (-d.y, d.x) * mat4x2 (a - c, v - c, w - c, x - c);
  }

  float G [4] [5];

  for (int j = 0; j < 4; ++ j) for (int i = 0; i < 5; ++ i) G [j] [i] = m [i] [j];
  triangle (A, V, W, g, h, i, G [0], G [1], G [2]);

  for (int j = 0; j < 4; ++ j) G [j] [0] = m [5] [j];
  triangle (A, W, X, g, i, j, G [0], G [2], G [3]);
}

void main ()
{
  vec3 x = g.x * Q [0];
  vec3 Y = g.y * Q [1];
  vec3 z = g.z * Q [2];
  vec3 X = g.x * Q [3];
  vec3 y = g.y * Q [4];
  vec3 Z = g.z * Q [5];

  vec3 U = O + X + y + z;
  vec3 V = O + x + Y + z;
  vec3 W = O + x + y + Z;

  vec4 h = project (U);
  vec4 i = project (V);
  vec4 j = project (W);

  vec2 u = pdivide (h);
  vec2 v = pdivide (i);
  vec2 w = pdivide (j);

  if (s) {
    N = normalize (cross (W - U, V - U));
    triangle (W, V, U, j, i, h,
       float [5] (dist (w, v, u), 0, 0, 1e9, 1e9),
       float [5] (0, dist (v, u, w), 0, 1e9, 1e9),
       float [5] (0, 0, dist (u, w, v), 1e9, 1e9));
    snub_segment (Q [4], W, U, j, h);
    snub_segment (Q [2], U, V, h, i);
  }
  else {
    vec3 T = O + x + y + z;
    vec4 g = project (T);
    vec2 t = pdivide (g);
    aspect (Q [0], V, T, W, i, g, j, v, t, w);
    aspect (Q [4], W, T, U, j, g, h, w, t, u);
    aspect (Q [2], U, T, V, h, g, i, u, t, v);
  }
}
