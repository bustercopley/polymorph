// -*- C++ -*-

void main ()
{
  vec3 rg = r * g;
  vec3 x0 = rg [0] * xs [0];
  vec3 y1 = rg [1] * xs [1];
  vec3 z0 = rg [2] * xs [2];
  vec3 x1 = rg [0] * xs [3];
  vec3 y0 = rg [1] * xs [4];
  vec3 z1 = rg [2] * xs [5];

  vec3 t = x0 + y0 + z0;

  vec3 a = dot (xs [0], t) * xs [0];
  vec3 b = dot (xs [4], t) * xs [4];
  vec3 c = dot (xs [2], t) * xs [2];

  vec3 mx = t + 0.5 * (x1 - x0);
  vec3 my = t + 0.5 * (y1 - y0);
  vec3 mz = t + 0.5 * (z1 - z0);

  float hx = length (my - a);
  float kx = length (mz - a);

  float hy = length (mz - b);
  float ky = length (mx - b);

  float hz = length (mx - c);
  float kz = length (my - c);

  mat4 pm = p * m;
  vec4 T = pm * vec4 (t, 1.0);
  vec4 U = pm * vec4 (x1 + y0 + z0, 1.0);
  vec4 V = pm * vec4 (x0 + y1 + z0, 1.0);
  vec4 W = pm * vec4 (x0 + y0 + z1, 1.0);
  vec4 A = pm * vec4 (a, 1.0);
  vec4 B = pm * vec4 (b, 1.0);
  vec4 C = pm * vec4 (c, 1.0);

  vec3 cp = color (xs [0]);
  vec3 cq = color (xs [4]);
  vec3 cr = color (xs [2]);

  segment (A, V, T, cp, hx);
  segment (A, T, W, cp, kx);
  segment (B, W, T, cq, hy);
  segment (B, T, U, cq, ky);
  segment (C, U, T, cr, hz);
  segment (C, T, V, cr, kz);
}
