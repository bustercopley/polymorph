#version 420

in vec4 x;
out vec3 xs;

void main()
{
  xs = x.xyz;
}
