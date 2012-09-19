// -*- C++ -*-
#ifndef opengl_h
#define opengl_h

#include "mswin.h"
#include <GL/gl.h>
#include "glext.h"
#include "wglext.h"

bool glprocs ();

#define GLPROCSTRINGIZE0(a) #a
#define GLPROC(type, name) (name = (type) ::wglGetProcAddress (GLPROCSTRINGIZE0(name)))

extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

#endif
