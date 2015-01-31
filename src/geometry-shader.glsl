// -*- C++ -*-

#version 420

layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 p;
uniform int w;
uniform int h;

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
out noperspective vec3 E;

vec3 O = m [3].xyz;


void vertex (vec3 x, vec4 p, vec3 e)
{
  E = e;
  X = x;
  gl_Position = p;
  EmitVertex ();
}

void triangle (vec3 A, vec3 B, vec3 C, vec4 x, vec4 y, vec4 z, mat3 e)
{
  vertex (A, x, e [0]);
  vertex (B, y, e [1]);
  vertex (C, z, e [2]);
  EndPrimitive ();
}

vec4 project (vec3 x)
{
  return p * vec4 (x, 1);
}

vec2 pdivide (vec4 s)
{
  return vec2 (w * s.x / s.w, h * s.y / s.w);
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

vec2 flip (vec3 A, vec3 U, vec3 V)
{
  vec3 e = U - A;
  vec3 v = V - U;
  return raster (U - v + (2 * dot (v, e) / dot (e, e)) * e);
}

void segment (vec3 A, vec3 W, vec3 X, vec4 c, vec4 d, vec4 e, vec2 a, vec2 u, vec2 v, vec2 w, vec2 x, vec2 y, vec2 z)
{
  vec2 k, l;
  if (dot (w - v, w - v) < 1) {
    k = perp (u, v);
    l = perp (y, z);
  }
  else {
    k = perp (v, w);
    l = perp (x, y);
  }
  triangle (A, W, X, c, d, e,
            mat3 (dot (k, a - v), dist (a, w, x), dot (l, a - y),
                  0, 0, dot (l, w - y),
                  dot (k, x - v), 0, 0));
}

void snub_segment (vec3 Q, vec3 U, vec3 V, vec4 y, vec4 z)
{
  N = Q;
  vec3 C = O + dot (U - O, Q) * Q;
  vec4 x = project (C);
  vec2 c = pdivide (x);
  vec2 t = flip (C, U, V);
  vec2 u = pdivide (y);
  vec2 v = pdivide (z);
  vec2 w = flip (C, V, U);
  vec2 k = perp (t, u);
  vec2 l = perp (v, w);
  triangle (C, U, V, x, y, z,
            mat3 (dot (k, c - u), dist (c, u, v), dot (l, c - w),
                  0, 0, dot (l, u - w),
                  dot (k, v - u), 0, 0));
  EndPrimitive ();
}

void aspect (vec3 Q, vec3 T, vec3 V, vec3 W, vec4 h, vec4 i, vec4 j, vec2 t, vec2 v, vec2 w)
{
  vec3 A = O + dot (T - O, Q) * Q;
  vec3 y = W - A;
  vec3 z = V - A;
  vec3 d = T - 2 * A;
  vec3 e = d + V;
  vec3 f = d + W;
  vec3 E = e * (2 / dot (e, e));
  vec3 F = f * (2 / dot (f, f));
  vec3 X = dot (e, y) * E - y;
  vec3 U = dot (f, z) * F - z;
  vec4 g = project (A);
  vec2 a = pdivide (g);
  vec2 x = raster (A + X);
  vec2 u = raster (A + U);
  vec2 r = raster (A + dot (e, U) * E - U);
  vec2 s = raster (A + dot (f, X) * F - X);
  N = Q;
  segment (A, V, T, g, i, h, a, r, x, v, t, w, u);
  segment (A, T, W, g, h, j, a, x, v, t, w, u, s);
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
              mat3 (0, 0, dist (w, v, u),
                    0, dist (v, u, w), 0,
                    dist (u, w, v), 0, 0));
    snub_segment (Q [4], W, U, j, h);
    snub_segment (Q [2], U, V, h, i);
  }
  else {
    vec3 T = O + x + y + z;
    vec4 g = project (T);
    vec2 t = pdivide (g);
    aspect (Q [0], T, V, W, g, i, j, t, v, w);
    aspect (Q [4], T, W, U, g, j, h, t, w, u);
    aspect (Q [2], T, U, V, g, h, i, t, u, v);
  }
}
