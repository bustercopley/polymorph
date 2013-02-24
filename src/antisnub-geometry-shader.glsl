// -*- C++ -*-
void main ()
{
  vec3 G = r * g;
  vec3 r = cross (S [0], S [1]);
  vec3 s = S [5];
  vec3 t = S [4];
  snub (s, t, G [0] * S [0], G [2] * S [2], G [1] * t, G [0] * (S [3] - 2 * (dot (S [3], r) / dot (r, r)) * r), G [2] * s, G [1] * S [1]);
}
