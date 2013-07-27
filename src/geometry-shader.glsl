// -*- C++ -*-

void segment (vec4 A, vec4 V, vec4 W, vec3 c, float h, float k, float l)
{
  vertex (A, c, vec3 (h, k, k));
  vertex (V, c, vec3 (0, 0, l));
  vertex (W, c, vec3 (0, l, 0));
  EndPrimitive ();
}

void aspect (vec3 x, vec3 y, vec3 z, vec3 Y, vec3 Z, vec3 S)
{
  vec3 e = color (S);
  vec3 t = x + y + z;
  vec3 v = Y - y;
  vec3 w = Z - z;
  vec3 a = dot (S, t) * S;
  vec3 d = a - t;
  float D = dot (d, d);
  float B = dot (v, v);
  float C = dot (w, w);
  float f = sq (dot (v, w));
  float h = sqrt (D - B / 4);
  float k = sqrt (D - C / 4);
  mat4 q = p * m;
  vec4 T = q * vec4 (t, 1);
  vec4 A = q * vec4 (a, 1);
  segment (A, q * vec4 (x + Y + z, 1), T, e, h, k, sqrt (B - f / C));
  segment (A, T, q * vec4 (x + y + Z, 1), e, k, h, sqrt (C - f / B));
}

void main ()
{
  vec3 G = r * g;
  vec3 x = G [0] * S [0];
  vec3 Y = G [1] * S [1];
  vec3 z = G [2] * S [2];
  vec3 X = G [0] * S [3];
  vec3 y = G [1] * S [4];
  vec3 Z = G [2] * S [5];

  aspect (x, y, z, Y, Z, S [0]);
  aspect (y, z, x, Z, X, S [4]);
  aspect (z, x, y, X, Y, S [2]);
}
