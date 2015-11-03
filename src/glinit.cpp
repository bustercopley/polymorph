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

const int pf_attribs [] = {
  WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
  WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
  WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
  WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
  WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
  WGL_COLOR_BITS_ARB, 24,
  WGL_SAMPLE_BUFFERS_ARB, GL_FALSE,
  //WGL_SAMPLES_ARB, 5,
  0, 0,
};

const int context_attribs [] = {
  WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
  WGL_CONTEXT_MINOR_VERSION_ARB, 2,
  WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
  0, 0,
};

// Get OpenGL function pointers (call with final OpenGL context current).
ALWAYS_INLINE inline bool get_glprocs ()
{
#define GLPROC_STRINGIZE(a) #a
#define DO_GLPROC(type, name) (name = (type) ::wglGetProcAddress (GLPROC_STRINGIZE(name))); if (! name) return false
#include "glprocs.inc"
#undef DO_GLPROC
  return true;
}

// Get OpenGL function pointers needed for context creation (call with legacy context current).
ALWAYS_INLINE inline void get_init_glprocs ()
{
  wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) ::wglGetProcAddress ("wglChoosePixelFormatARB");
  wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) ::wglGetProcAddress ("wglCreateContextAttribsARB");
}

LRESULT CALLBACK InitWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg != WM_NCCREATE) return ::DefWindowProc (hwnd, msg, wParam, lParam);
  PIXELFORMATDESCRIPTOR pfd = { sizeof pfd, 1, PFD_SUPPORT_OPENGL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };
  HDC hdc = ::GetDC (hwnd);
  int pf = ::ChoosePixelFormat (hdc, & pfd);
  ::SetPixelFormat (hdc, pf, & pfd);
  HGLRC hglrc = ::wglCreateContext (hdc);
  ::wglMakeCurrent (hdc, hglrc);
  // Get the function pointers.
  get_init_glprocs ();
  ::wglMakeCurrent (NULL, NULL);
  ::wglDeleteContext (hglrc);
  ::ReleaseDC (hwnd, hdc);
  return FALSE; // Abort window creation.
}

bool glinit (HINSTANCE hInstance)
{
  // Create legacy OpenGL window to get function pointers needed to create a proper OpenGL window.
  // The window does not survive creation. See InitWndProc for details.
  WNDCLASSEX init_wndclass = { sizeof (WNDCLASSEX), 0, & InitWndProc, 0, 0, hInstance, NULL, NULL, NULL, NULL, WC_GLINIT, NULL };
  ::RegisterClassEx (& init_wndclass);
  ::CreateWindowEx (0, WC_GLINIT, TEXT (""), 0,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, hInstance, NULL);
  return wglChoosePixelFormatARB && wglCreateContextAttribsARB;
}

HGLRC install_rendering_context (HDC hdc)
{
  // Set up OpenGL rendering context.
  int pf;
  UINT pfcount;
  if (! wglChoosePixelFormatARB (hdc, pf_attribs, NULL, 1, & pf, & pfcount) ||
      ! pfcount ||
      ! ::SetPixelFormat (hdc, pf, NULL)) {
    return NULL;
  }

  if (HGLRC hglrc = wglCreateContextAttribsARB (hdc, NULL, context_attribs)) {
    if (::wglMakeCurrent (hdc, hglrc)) {
      if (get_glprocs ()) {
        wglSwapIntervalEXT (1);
        return hglrc;
      }
      ::wglMakeCurrent (NULL, NULL);
    }
    ::wglDeleteContext (hglrc);
  }

  return NULL;
}
