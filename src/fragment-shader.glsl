#version 420

in vec4 gc;
in vec3 ed;
out vec4 c;

float amplify (float d, float scale, float offset)
{
  d = scale * d + offset;
  d = clamp (d, 0, 1);
  d = 1 - exp2 (-2 * d * d);
  return d;
}

void main ()
{
  float d = min (min (ed [0], ed [1]), ed [2]);
  c = amplify (d, 50, -1.0) * gc;
}
