// Copyright 2012-2019 Richard Copley
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

#include "mswin.h"
#include "glinit.h"
#include "compiler.h"

// Define OpenGL function pointer variables.
#define DO_GLPROC(type,name) type name = nullptr
#include "glprocs.inc"
#undef DO_GLPROC

#define WC_GLINIT TEXT ("g")

// These functions are needed during initialisation; they are handled specially.
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

// Pixel format attributes for up-level opengl context.
const int pfattribs [] = {
  WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
  WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
  WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
  WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
  WGL_SWAP_METHOD_ARB, WGL_SWAP_COPY_ARB,
  WGL_COLOR_BITS_ARB, 24,
  WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
  WGL_SAMPLES_ARB, 4,
  0, 0,
};

// Rendering context attributes for up-level opengl context.
const int rcattribs [] = {
  WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
  WGL_CONTEXT_MINOR_VERSION_ARB, 3,
  WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
  0, 0,
};

template <typename F> ALWAYS_INLINE
inline bool getproc (F & f, const char * name) {
  // Cast via void (*) () to suppress "-Wcast-function-type" warning.
  f = (F) (void (*) ()) ::wglGetProcAddress (name);
#ifndef TINY
  return f != nullptr;
#else
  return true;
#endif
}

// Get OpenGL function pointers (call with final OpenGL context current).
ALWAYS_INLINE inline bool get_glprocs ()
{
#define GLPROC_STRINGIZE(a) #a
#define DO_GLPROC(t, f) if (! getproc (f, GLPROC_STRINGIZE (f))) return false
#include "glprocs.inc"
#undef DO_GLPROC
  return true;
}

LRESULT CALLBACK InitWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg != WM_NCCREATE) return ::DefWindowProc (hwnd, msg, wParam, lParam);
  // Create a legacy OpenGL context and make it current in this window.
  PIXELFORMATDESCRIPTOR pfd = {sizeof pfd, 1, PFD_SUPPORT_OPENGL,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  HDC hdc = ::GetDC (hwnd);
  int pf = ::ChoosePixelFormat (hdc, & pfd);
  ::SetPixelFormat (hdc, pf, & pfd);
  HGLRC hglrc = ::wglCreateContext (hdc);
  ::wglMakeCurrent (hdc, hglrc);
  // Get OpenGL function pointers needed for context creation.
  getproc (wglChoosePixelFormatARB, "wglChoosePixelFormatARB");
  getproc (wglCreateContextAttribsARB, "wglCreateContextAttribsARB");
  // Destroy the rendering context.
  ::wglMakeCurrent (nullptr, nullptr);
  ::wglDeleteContext (hglrc);
  ::ReleaseDC (hwnd, hdc);
  // Abort window creation.
  return FALSE;
}

bool glinit (HINSTANCE hInstance)
{
  // Create a window with a legacy OpenGL rendering context, to get
  // the WGL function pointers needed to create an up-level rendering
  // context. The window does not survive creation. See InitWndProc.
  WNDCLASSEX init_wndclass = {sizeof (WNDCLASSEX), 0, &InitWndProc, 0, 0,
    hInstance, nullptr, nullptr, nullptr, nullptr, WC_GLINIT, nullptr};
  ::RegisterClassEx (&init_wndclass);
  ::CreateWindowEx (0, WC_GLINIT, TEXT (""), 0,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    nullptr, nullptr, hInstance, nullptr);
  // Return true if we succeeded in getting the function pointers.
  return wglChoosePixelFormatARB && wglCreateContextAttribsARB;
}

HGLRC install_rendering_context (HDC hdc)
{
  // Create up-level OpenGL rendering context and make it current in this DC.
  int pf;
  UINT pfcount;
  if (! wglChoosePixelFormatARB (hdc, pfattribs, nullptr, 1, & pf, & pfcount)
      || ! pfcount
      || ! ::SetPixelFormat (hdc, pf, nullptr)) {
    return nullptr;
  }

  if (HGLRC hglrc = wglCreateContextAttribsARB (hdc, nullptr, rcattribs)) {
    if (::wglMakeCurrent (hdc, hglrc)) {
      if (get_glprocs ()) {
        wglSwapIntervalEXT (1);
        return hglrc;
      }
      ::wglMakeCurrent (nullptr, nullptr);
    }
    ::wglDeleteContext (hglrc);
  }

  return nullptr;
}
