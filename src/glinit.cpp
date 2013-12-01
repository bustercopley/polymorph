#include "glinit.h"

#define DO_GLPROC(type,name) type name = nullptr
#include "glprocs.inc"
#undef DO_GLPROC

#define GLPROCSTRINGIZE0(a) #a
#define GLPROC(type, name) (name = (type) ::wglGetProcAddress (GLPROCSTRINGIZE0(name)))
#define VALIDGLPROC(type, name) if (! (GLPROC (type, name))) return false

bool get_glprocs ()
{
#define DO_GLPROC(type, name) VALIDGLPROC (type, name)
#include "glprocs.inc"
#undef DO_GLPROC
  return true;
}
