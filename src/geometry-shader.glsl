// -*- C++ -*-

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
  segment (A, V, T, h [0], h [1], h [3]);
  segment (A, T, W, h [1], h [0], h [2]);
}

void main ()
{
  vec3 G = r * g.xyz;
  vec3 x = G [0] * S [0];
  vec3 y = G [1] * S [4];
  vec3 z = G [2] * S [2];
  vec3 t = x + y + z;
  mat4 q = p * m;
  vec4 T = q * vec4 (t, 1);
  vec4 U = q * vec4 (y + z + G [0] * S [3], 1);
  vec4 V = q * vec4 (z + x + G [1] * S [1], 1);
  vec4 W = q * vec4 (x + y + G [2] * S [5], 1);
  aspect (q * vec4 (dot (S [0], t) * S [0], 1), T, V, W, S [0], r * h [0]);
  aspect (q * vec4 (dot (S [4], t) * S [4], 1), T, W, U, S [4], r * h [1]);
  aspect (q * vec4 (dot (S [2], t) * S [2], 1), T, U, V, S [2], r * h [2]);
}
