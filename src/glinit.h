// -*- C++ -*-

#ifndef glinit_h
#define glinit_h

#include "mswin.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

// Declare OpenGL function pointer variables, defined in glinit.cpp.
#define DO_GLPROC(type,name) extern type name
#include "glprocs.inc"
#undef DO_GLPROC

bool glinit (HINSTANCE hInstance);
HGLRC install_rendering_context (HDC hdc);

#endif
