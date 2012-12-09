// -*- C++ -*-

float sq (float x) { return x * x; }

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

  vec3 ta = a - t;
  vec3 tb = b - t;
  vec3 tc = c - t;

  vec3 tu = x1 - x0;
  vec3 tv = y1 - y0;
  vec3 tw = z1 - z0;

  float aa = dot (ta, ta);
  float bb = dot (tb, tb);
  float cc = dot (tc, tc);

  float uu = dot (tu, tu);
  float vv = dot (tv, tv);
  float ww = dot (tw, tw);

  float av = sq (dot (ta, tv));
  float aw = sq (dot (ta, tw));

  float bu = sq (dot (tb, tu));
  float bw = sq (dot (tb, tw));

  float cu = sq (dot (tc, tu));
  float cv = sq (dot (tc, tv));

  float vw = sq (dot (tv, tw));
  float wu = sq (dot (tw, tu));
  float uv = sq (dot (tu, tv));

  float ru = 1 / uu;
  float rv = 1 / vv;
  float rw = 1 / ww;

  float hx = sqrt (aa - av * rv);
  float hy = sqrt (bb - bw * rw);
  float hz = sqrt (cc - cu * ru);

  float kx = sqrt (aa - aw * rw);
  float ky = sqrt (bb - bu * ru);
  float kz = sqrt (cc - cv * rv);

  float h1x = sqrt (ww - vw * rv);
  float h1y = sqrt (uu - wu * rw);
  float h1z = sqrt (vv - uv * ru);

  float k1x = sqrt (vv - vw * rw);
  float k1y = sqrt (ww - wu * ru);
  float k1z = sqrt (uu - uv * rv);

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

  segment (A, V, T, cp, hx, kx, k1x);
  segment (A, T, W, cp, kx, hx, h1x);
  segment (B, W, T, cq, hy, ky, k1y);
  segment (B, T, U, cq, ky, hy, h1y);
  segment (C, U, T, cr, hz, kz, k1z);
  segment (C, T, V, cr, kz, hz, h1z);
}
