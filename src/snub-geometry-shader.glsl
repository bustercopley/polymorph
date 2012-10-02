// -*- C++ -*-
void main ()
{
  vec3 x0 = g [0] * xs [0];
  vec3 y1 = g [1] * xs [1];
  vec3 z0 = g [2] * xs [2];
  vec3 x1 = g [0] * xs [3];
  vec3 y0 = g [1] * xs [4];
  vec3 z1 = g [2] * xs [5];

  vec3 u = x1 + y0 + z0;
  vec3 v = x0 + y1 + z0;
  vec3 w = x0 + y0 + z1;

  vec3 t = normalize (cross (w - u, v - u));

  vec3 cy = color (xs [4]);
  vec3 cz = color (xs [2]);
  vec3 ct = color (t);

  mat4 pm = p * m;

  vec4 U = pm * vec4 (r * u, 1.0);
  vec4 V = pm * vec4 (r * v, 1.0);
  vec4 W = pm * vec4 (r * w, 1.0);
  vec4 B = pm * vec4 (r * dot (xs [4], u) * xs [4], 1.0);
  vec4 C = pm * vec4 (r * dot (xs [2], u) * xs [2], 1.0);
  triangle (U, V, W, ct);
  segment (B, W, U, cy);
  segment (C, U, V, cz);
}
