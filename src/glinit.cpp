#include "glinit.h"

#define DO_GLPROC(type,name) type name = nullptr
#include "glprocs.inc"
DO_GLPROC (PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);
DO_GLPROC (PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);
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

bool get_initial_wglprocs (HINSTANCE hInstance);

HGLRC initialize_opengl (HINSTANCE hInstance, HDC hdc)
{
  int pf_attribs [] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
    WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_DEPTH_BITS_ARB, 4,
    WGL_SAMPLE_BUFFERS_ARB, GL_FALSE,
    //WGL_SAMPLES_ARB, 5,
    0, 0,
  };

  int context_attribs [] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 2,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0, 0,
  };

  HGLRC hglrc;
  const UINT pfcountmax = 256;
  int pfs [pfcountmax];
  UINT pfcount;
  if (get_initial_wglprocs (hInstance)
      && wglChoosePixelFormatARB (hdc, pf_attribs, nullptr, pfcountmax, pfs, & pfcount)
      && pfcount
      && pfcount <= pfcountmax
      && ::SetPixelFormat (hdc, pfs [0], nullptr)
      && (hglrc = wglCreateContextAttribsARB (hdc, nullptr, context_attribs))
      && ::wglMakeCurrent (hdc, hglrc)
      && get_glprocs ()) {
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    GLPROC (PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
    if (wglSwapIntervalEXT) {
      wglSwapIntervalEXT (1);
    }
    return hglrc;
  }
  return nullptr;
}

bool get_initial_wglprocs (HINSTANCE hInstance)
{
  // To obtain an OpenGL 3 pixel format, we need to call wglChoosePixelFormatARB, but
  // first we must obtain the address of that function by calling using wglGetProcAddress,
  // which requires that an OpenGL rendering context is current, which requires a device
  // context that supports OpenGL, which requires a window with an OpenGL pixel format.

  // Bootstrap the process with a legacy OpenGL pixel format. According to MSDN,
  // "Once a window's pixel format is set, it cannot be changed", so the window
  // and associated resources are of no further use and are destroyed here.

  WNDCLASS wc;
  ::ZeroMemory (& wc, sizeof wc);
  wc.hInstance = hInstance;
  wc.lpfnWndProc = & ::DefWindowProc;
  wc.lpszClassName = "GLinit";
  if (ATOM atom = ::RegisterClass (& wc))
  {
    if (HWND hwnd = ::CreateWindowEx (0,
         MAKEINTATOM (atom), nullptr, 0,
         CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
         HWND_MESSAGE, nullptr, hInstance, nullptr))
    {
      PIXELFORMATDESCRIPTOR pfd;
      ::ZeroMemory (& pfd, sizeof pfd);
      pfd.nSize = sizeof pfd;
      pfd.dwFlags = PFD_SUPPORT_OPENGL;
      if (HDC dc = ::GetDC (hwnd))
      {
        if (int pf = ::ChoosePixelFormat (dc, & pfd))
        {
          if (::SetPixelFormat (dc, pf, & pfd))
          {
            if (HGLRC rc = ::wglCreateContext (dc))
            {
              if (::wglMakeCurrent (dc, rc))
              {
                VALIDGLPROC(PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);
                VALIDGLPROC(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);
                ::wglMakeCurrent (nullptr, nullptr);
              }
              ::wglDeleteContext (rc);
            }
          }
        }
        ::ReleaseDC (hwnd, dc);
      }
      ::DestroyWindow (hwnd);
    }
    ::UnregisterClass (MAKEINTATOM (atom), hInstance);
  }
  return !! wglChoosePixelFormatARB;
}
