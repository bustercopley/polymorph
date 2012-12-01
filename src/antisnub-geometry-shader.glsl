// -*- C++ -*-
void main ()
{
  vec3 yz = cross (xs [0], xs [1]);

  vec3 rg = r * g;
  vec3 x0 = rg [0] * xs [0];
  vec3 y1 = rg [1] * xs [1];
  vec3 z0 = rg [2] * xs [5];
  vec3 x1 = rg [0] * (xs [3] - 2.0 * (dot (xs [3], yz) / dot (yz, yz)) * yz);
  vec3 y0 = rg [1] * xs [4];
  vec3 z1 = rg [2] * xs [2];

  vec3 u = x1 + y0 + z0;
  vec3 v = x0 + y1 + z0;
  vec3 w = x0 + y0 + z1;

  vec3 t = normalize (cross (v - u, w - u));

  vec3 cy = color (xs [4]);
  vec3 cz = color (xs [5]);
  vec3 ct = color (t);

  mat4 pm = p * m;

  vec3 b = dot (xs [4], u) * xs [4];
  vec3 c = dot (xs [5], u) * xs [5];

  vec3 mx = 0.5 * (v + w);
  vec3 my = 0.5 * (w + u);
  vec3 mz = 0.5 * (u + v);

  float h = length (b - my);
  float k = length (c - mz);

  float hx = length (u - mx);
  float hy = length (v - my);
  float hz = length (w - mz);

  vec4 U = pm * vec4 (u, 1.0);
  vec4 V = pm * vec4 (v, 1.0);
  vec4 W = pm * vec4 (w, 1.0);
  vec4 B = pm * vec4 (b, 1.0);
  vec4 C = pm * vec4 (c, 1.0);

  triangle (U, W, V, ct, hx, hy, hz);
  segment (B, U, W, cy, h);
  segment (C, V, U, cz, k);
}
