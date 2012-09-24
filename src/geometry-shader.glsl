// -*- C++ -*-
void segment (vec4 T, vec4 A, vec4 V, vec4 W, vec4 c)
{
  gc = c;
  ed = vec3 (0.0, 1.0, 1.0);
  gl_Position = V;
  EmitVertex ();

  gc = c;
  ed = vec3 (0.0, 1.0, 1.0);
  gl_Position = T;
  EmitVertex ();

  gc = c;
  ed = vec3 (1.0, 1.0, 1.0);
  gl_Position = A;
  EmitVertex ();

  gc = c;
  ed = vec3 (0.0, 1.0, 1.0);
  gl_Position = W;
  EmitVertex ();

  EndPrimitive ();
}
void main ()
{
  vec3 rg = r * g;
  vec3 x0 = rg [0] * xs [0];
  vec3 y1 = rg [1] * xs [1];
  vec3 z0 = rg [2] * xs [2];
  vec3 x1 = rg [0] * xs [3];
  vec3 y0 = rg [1] * xs [4];
  vec3 z1 = rg [2] * xs [5];

  mat4 pm = p * m;
  vec3 t = x0 + y0 + z0;
  vec4 T = pm * vec4 (t, 1.0);
  vec4 U = pm * vec4 (x1 + y0 + z0, 1.0);
  vec4 V = pm * vec4 (x0 + y1 + z0, 1.0);
  vec4 W = pm * vec4 (x0 + y0 + z1, 1.0);
  vec4 A = pm * vec4 (dot (xs [0], t) * xs [0], 1.0);
  vec4 B = pm * vec4 (dot (xs [4], t) * xs [4], 1.0);
  vec4 C = pm * vec4 (dot (xs [2], t) * xs [2], 1.0);

  vec4 cp = color (xs [0]);
  vec4 cq = color (xs [4]);
  vec4 cr = color (xs [2]);

  segment (T, A, V, W, cp);
  segment (T, B, W, U, cq);
  segment (T, C, U, V, cr);
}
