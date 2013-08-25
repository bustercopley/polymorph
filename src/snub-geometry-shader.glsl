// -*- C++ -*-
void segment (vec3 S, vec4 A, vec4 V, vec4 W, float h)
{
  N = color (S);
  vertex (A, vec3 (h, 1, 1));
  vertex (V, vec3 (0, 1, 1));
  vertex (W, vec3 (0, 1, 1));
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

  vec3 P = w - v;
  vec3 Q = u - w;
  vec3 R = v - u;

  mat4 q = p * m;

  vec4 U = q * vec4 (u, 1);
  vec4 V = q * vec4 (v, 1);
  vec4 W = q * vec4 (w, 1);

  float A = dot (P, P);
  float B = dot (Q, Q);
  float C = dot (R, R);

  float D = dot (Q, R);
  float E = dot (R, P);
  float F = dot (P, Q);

  float s = dot (S [4], u);
  float t = dot (S [2], u);

  N = color (normalize (cross (v - u, u - w)));
  vertex (W, vec3 (0, 0, sqrt (B - D * D / C)));
  vertex (V, vec3 (0, sqrt (A - F * F / B), 0));
  vertex (U, vec3 (sqrt (C - E * E / A), 0, 0));
  EndPrimitive ();

  segment (S [4], q * vec4 (s * S [4], 1), W, U, sqrt (r * r - s * s - B / 4));
  segment (S [2], q * vec4 (t * S [2], 1), U, V, sqrt (r * r - t * t - C / 4));
}

