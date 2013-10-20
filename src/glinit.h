// -*- C++ -*-
#ifndef glinit_h
#define glinit_h

#ifndef mswin_h
#error Must include "mswin.h" before including "glinit.h"
#endif

#include <GL/gl.h>
#include "glext.h"
#include "wglext.h"

HGLRC initialize_opengl (HINSTANCE hInstance, HDC hdc);

#define DO_GLPROC(type,name) extern type name
DO_GLPROC (PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);
DO_GLPROC (PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
#include "glprocs.inc"
#undef DO_GLPROC

#endif
