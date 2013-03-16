#include "mswin.h"
#include "glinit.h"

PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
//PFNGLDELETESHADERPROC glDeleteShader = nullptr;
//PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
//PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;
PFNGLUNIFORM4FVPROC glUniform4fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
//PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
//PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = nullptr;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = nullptr;
PFNGLBLENDEQUATIONPROC glBlendEquation = nullptr;

bool get_glprocs ();
bool get_wglChoosePixelFormatARB (HINSTANCE hInstance);

HGLRC initialize_opengl (HINSTANCE hInstance, HDC hdc)
{
  int ilist [] = {
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
  float flist [] = { 0, 0, };

  HGLRC hglrc;
  const UINT pfcountmax = 256;
  int pfs [pfcountmax];
  UINT pfcount;
  if (get_wglChoosePixelFormatARB (hInstance)
      && wglChoosePixelFormatARB (hdc, ilist, flist, pfcountmax, pfs, & pfcount)
      && pfcount
      && pfcount <= pfcountmax
      && ::SetPixelFormat (hdc, pfs [0], nullptr)
      && (hglrc = ::wglCreateContext (hdc))
      && ::wglMakeCurrent (hdc, hglrc)
      && get_glprocs ()) {
    if (wglSwapIntervalEXT) {
      wglSwapIntervalEXT (1);
    }
    return hglrc;
  }
  return nullptr;
}

#define GLPROCSTRINGIZE0(a) #a
#define GLPROC(type, name) (name = (type) ::wglGetProcAddress (GLPROCSTRINGIZE0(name)))
#define VALIDGLPROC(type, name) if (! (GLPROC (type, name))) return false

bool get_glprocs ()
{
  GLPROC (PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
  VALIDGLPROC (PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
  VALIDGLPROC (PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
  VALIDGLPROC (PFNGLGENBUFFERSPROC, glGenBuffers);
  VALIDGLPROC (PFNGLBINDBUFFERPROC, glBindBuffer);
  VALIDGLPROC (PFNGLBUFFERDATAPROC, glBufferData);
  VALIDGLPROC (PFNGLCREATESHADERPROC, glCreateShader);
  //VALIDGLPROC (PFNGLDELETESHADERPROC, glDeleteShader);
  //VALIDGLPROC (PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced);
  VALIDGLPROC (PFNGLSHADERSOURCEPROC, glShaderSource);
  VALIDGLPROC (PFNGLCOMPILESHADERPROC, glCompileShader);
  VALIDGLPROC (PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
  VALIDGLPROC (PFNGLGETSHADERIVPROC, glGetShaderiv);
  VALIDGLPROC (PFNGLCREATEPROGRAMPROC, glCreateProgram);
  //VALIDGLPROC (PFNGLDELETEPROGRAMPROC, glDeleteProgram);
  VALIDGLPROC (PFNGLATTACHSHADERPROC, glAttachShader);
  VALIDGLPROC (PFNGLLINKPROGRAMPROC, glLinkProgram);
  VALIDGLPROC (PFNGLUSEPROGRAMPROC, glUseProgram);
  VALIDGLPROC (PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
  VALIDGLPROC (PFNGLGETPROGRAMIVPROC, glGetProgramiv);
  VALIDGLPROC (PFNGLUNIFORM1FPROC, glUniform1f);
  VALIDGLPROC (PFNGLUNIFORM1IPROC, glUniform1i);
  VALIDGLPROC (PFNGLUNIFORM3FVPROC, glUniform3fv);
  VALIDGLPROC (PFNGLUNIFORM4FVPROC, glUniform4fv);
  VALIDGLPROC (PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
  VALIDGLPROC (PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
  //VALIDGLPROC (PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor);
  VALIDGLPROC (PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
  VALIDGLPROC (PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
  //VALIDGLPROC (PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
  VALIDGLPROC (PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation);
  VALIDGLPROC (PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
  VALIDGLPROC (PFNGLBLENDEQUATIONPROC, glBlendEquation);
  return true;
}

bool get_wglChoosePixelFormatARB (HINSTANCE hInstance)
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
  wc.lpszClassName = "PolymorphDummy";
  if (ATOM atom = ::RegisterClass (& wc))
  {
    if (HWND hwnd = ::CreateWindowEx (0,
         MAKEINTATOM (atom), "PolymorphDummy", 0,
         CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
         nullptr, nullptr, hInstance, nullptr))
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
                GLPROC(PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);
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
