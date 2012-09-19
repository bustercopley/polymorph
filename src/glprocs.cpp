#include "glprocs.h"

PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

#define VALIDGLPROC(type, name) if (! (GLPROC (type, name))) return false;

bool glprocs ()
{
  VALIDGLPROC (PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
  return true;
}
