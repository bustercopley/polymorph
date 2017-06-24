// -*- C++ -*-

// Copyright 2012-2017 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
