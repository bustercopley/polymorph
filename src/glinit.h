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

extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLCREATESHADERPROC glCreateShader;
//extern PFNGLDELETESHADERPROC glDeleteShader;
//extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
//extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLUNIFORM1FPROC glUniform1f;
//extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
//extern PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
//extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
//extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLBLENDEQUATIONPROC glBlendEquation;

#endif
