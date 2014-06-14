// -*- C++ -*-

#ifndef glinit_h
#define glinit_h

#include "mswin.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

bool get_glprocs ();

#define DO_GLPROC(type,name) extern type name
#include "glprocs.inc"
#undef DO_GLPROC

#endif
