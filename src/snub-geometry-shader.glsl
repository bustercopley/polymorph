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
  vec3 u = x1 + y0 + z0;
  vec3 v = x0 + y1 + z0;
  vec3 w = x0 + y0 + z1;
  vec3 t = normalize (cross (w - u, v - u));
  vec3 cy = color (xs [4]);
  vec3 cz = color (xs [2]);
  vec3 ct = color (t);
  vec3 P = w - v;
  vec3 Q = u - w;
  vec3 R = v - u;
  vec3 b = dot (xs [4], u) * xs [4];
  vec3 c = dot (xs [2], u) * xs [2];
  vec3 bu = u - b;
  vec3 cv = v - c;
  float PP = dot (P, P);
  float QQ = dot (Q, Q);
  float RR = dot (R, R);
  float rQQ = 1 / QQ;
  float rRR = 1 / RR;
  float QR = dot (Q, R);
  float RP = dot (R, P);
  float PQ = dot (P, Q);
  float hx = sqrt (RR - RP * RP / PP);
  float hy = sqrt (PP - PQ * PQ * rQQ);
  float hz = sqrt (QQ - QR * QR * rRR);
  float tu = dot (bu, Q);
  float tv = dot (cv, R);
  float h = sqrt (dot (bu, bu) - tu * tu * rQQ);
  float k = sqrt (dot (cv, cv) - tv * tv * rRR);
  mat4 pm = p * m;
  vec4 U = pm * vec4 (u, 1);
  vec4 V = pm * vec4 (v, 1);
  vec4 W = pm * vec4 (w, 1);
  vec4 B = pm * vec4 (b, 1);
  vec4 C = pm * vec4 (c, 1);
  triangle (W, V, U, ct, hz, hy, hx);
  segment (B, W, U, cy, h);
  segment (C, U, V, cz, k);
}
