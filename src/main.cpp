#include "mswin.h"
#include "model.h"
#include "random.h"
#include "cmdline.h"
#include "graphics.h"
#include "config.h"
#include "memory.h"
#include "vector.h"
#include "glprocs.h"
#include "compiler.h"
#include "tinyscheme-config.h"
#include <scheme-private.h>
#include <scheme.h>
#include <cstdint>

#define IDT_TIMER 1

// Is x one of the space-separated strings in xs?
template <typename ConstIterator>
bool in (const ConstIterator x, ConstIterator xs)
{
  while (* xs)
  {
    ConstIterator t = x;
    while (* t && * t == * xs) { ++ xs; ++ t; }
    if (! * t && (! * xs || * xs == ' ')) return true;
    while (* xs && * xs != ' ') ++ xs;
    if (* xs) ++ xs;
  }
  return false;
}

struct window_struct_t
{
  model_t model;
  HDC hdc;
  HGLRC hglrc;
  POINT initial_cursor_position;
  int width, height;
  run_mode_t mode;
};

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  bool close_window = false;
  bool call_def_window_proc = false;
  if (msg == WM_CREATE) {
    const CREATESTRUCT * cs = reinterpret_cast <CREATESTRUCT *> (lParam);
    window_struct_t * ws = reinterpret_cast <window_struct_t *> (cs->lpCreateParams);
    ::SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (ws));
    ::GetCursorPos (& ws->initial_cursor_position);
    ws->width = cs->cx;
    ws->height = cs->cy;
    return 0;
  }
  else if (window_struct_t * ws = reinterpret_cast <window_struct_t *> (::GetWindowLongPtr (hwnd, GWLP_USERDATA))) {
    switch (msg) {
    case WM_PAINT:
      ::ValidateRect (hwnd, nullptr);
      break;
    case WM_TIMER:
      {
        ws->model.proceed ();
        clear ();
        ws->model.draw ();
        ::SwapBuffers (ws->hdc);
      }
      break;

    case WM_SETCURSOR:
      ::SetCursor (ws->mode == fullscreen ? nullptr : (::LoadCursor (nullptr, IDC_ARROW)));
      break;

    case WM_MOUSEMOVE:
      if (ws->mode == fullscreen) {
        // Compare the current mouse position with the one stored in the window struct.
        POINT current { LOWORD (lParam), HIWORD (lParam) };
        ::ClientToScreen (hwnd, & current);
        SHORT dx = current.x - ws->initial_cursor_position.x;
        SHORT dy = current.y - ws->initial_cursor_position.y;
        close_window = dx > 10 || dy > 10 || dx * dx + dy * dy  > 100;
      }
      break;

    case WM_KEYDOWN: case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
      close_window = ws->mode == fullscreen || ws->mode == special;
      break;

    case WM_ACTIVATE: case WM_ACTIVATEAPP: case WM_NCACTIVATE:
      close_window = ws->mode == fullscreen && LOWORD (wParam) == WA_INACTIVE;
      call_def_window_proc = true;
      break;

    case WM_SYSCOMMAND:
      call_def_window_proc = (ws->mode != fullscreen || wParam != SC_SCREENSAVE) && wParam != SC_CLOSE;
      break;

    case WM_CLOSE:
      ::DestroyWindow (hwnd);
      break;

    case WM_DESTROY:
      ::wglMakeCurrent (nullptr, nullptr);
      ::wglDeleteContext (ws->hglrc);
      ::PostQuitMessage (0);
      break;

    default:
      call_def_window_proc = true;
      break;
    }
  }
  else call_def_window_proc = true;

  if (close_window) {
    ::KillTimer (hwnd, IDT_TIMER);
    ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  }
  return call_def_window_proc ? ::DefWindowProc (hwnd, msg, wParam, lParam) : 0;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, int)
{
  // Parse the command line.

  HWND parent = 0;
  run_mode_t mode = parse_command_line (lpszCmdLine, & parent);
  if (mode == configure) {
    ::MessageBox (parent, usr::message, usr::message_window_name, MB_OK | MB_ICONASTERISK);
    return 0;
  }

  // Dummy rendering context to get the address of wglChoosePixelFormatARB.
  {
    const char * name = "PolymorphTemp";
    WNDCLASS wc;
    ::ZeroMemory (& wc, sizeof wc);
    wc.hInstance = hInstance;
    wc.lpfnWndProc = & ::DefWindowProc;
    wc.lpszClassName = name;
    ATOM atom = ::RegisterClass (& wc);
    if (! atom) return 1;
    HWND wnd = ::CreateWindowEx (0, MAKEINTATOM (atom), name, 0,
                                 CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                                 0, 0, hInstance, 0);
    if (! wnd) return 1;
    HDC dc = ::GetDC (wnd);
    PIXELFORMATDESCRIPTOR pfd;
    ::ZeroMemory (& pfd, sizeof pfd);
    pfd.nSize = sizeof pfd;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    int pf = ::ChoosePixelFormat (dc, & pfd);
    if (! pf) return 1;
    if (! ::SetPixelFormat (dc, pf, 0)) return 1;
    HGLRC rc = ::wglCreateContext (dc);
    if (! rc) return 1;
    if (! ::wglMakeCurrent (dc, rc)) return 1;
    // Get all needed gl function pointers while we're here.
    if (! glprocs ()) return 1;
    ::wglMakeCurrent (dc, 0);
    ::wglDeleteContext (rc);
    ::ReleaseDC (wnd, dc);
    ::DestroyWindow (wnd);
    ::UnregisterClass (MAKEINTATOM (atom), hInstance);
  }

  // Create the screen saver window.

  DWORD style;
  DWORD ex_style;
  int left, top, width, height;

  if (mode == embedded) {
    RECT rect;
    ::GetClientRect (parent, & rect);
    left = 0;
    top = 0;
    width = rect.right;
    height = rect.bottom;
    style = WS_CHILD | WS_VISIBLE;
    ex_style = 0;
  }
  else {
    left = ::GetSystemMetrics (SM_XVIRTUALSCREEN);
    top = ::GetSystemMetrics (SM_YVIRTUALSCREEN);
    width = ::GetSystemMetrics (SM_CXVIRTUALSCREEN);
    height = ::GetSystemMetrics (SM_CYVIRTUALSCREEN);
    style = WS_POPUP | WS_VISIBLE;
    ex_style = ((mode == fullscreen) ? WS_EX_TOPMOST : 0);
  }

  WNDCLASS wc;
  ::ZeroMemory (& wc, sizeof wc);
  wc.lpfnWndProc = & WndProc;
  wc.cbWndExtra = 8;
  wc.hInstance = hInstance;
  wc.hIcon = ::LoadIcon (hInstance, MAKEINTRESOURCE (257));
  wc.lpszClassName = usr::window_class_name;
  ATOM atom = ::RegisterClass (& wc);
  if (! atom) return 1;
  window_struct_t ws ALIGNED16;
  ws.mode = mode;
                                                //   HWND CreateWindowEx
  HWND hwnd = ::CreateWindowEx (ex_style,       //     (DWORD dwExStyle,
        MAKEINTATOM (atom),                     //      LPCTSTR lpClassName,
        usr::window_name,                       //      LPCTSTR lpWindowName,
        style,                                  //      DWORD dwStyle,
        left,                                   //      int x,
        top,                                    //      int y,
        width,                                  //      int nWidth,
        height,                                 //      int nHeight,
        parent,                                 //      HWND hWndParent,
        0,                                      //      HMENU hMenu,
        hInstance,                              //      HINSTANCE hInstance,
        & ws);                                  //      LPVOID lpParam);

  if (! hwnd) return 1;

  ws.hdc = ::GetDC (hwnd);
  if (! ws.hdc) return 1;

  int ilist [] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
    WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
    WGL_SAMPLES_ARB, 5,
    0, 0,
  };

  float flist [] = { 0, 0, };
  enum { pfcountmax = 256 };
  UINT pfcount;
  int pfs [pfcountmax];
  bool status = wglChoosePixelFormatARB (ws.hdc, ilist, flist, pfcountmax, pfs, & pfcount);
  if (! (status && pfcount && pfcount < pfcountmax)) return 1;
  if (! ::SetPixelFormat (ws.hdc, pfs [0], 0)) return 1;
  ws.hglrc = ::wglCreateContext (ws.hdc);
  if (! ws.hglrc) return 1;
  if (! ::wglMakeCurrent (ws.hdc, ws.hglrc)) {
    ::wglDeleteContext (ws.hglrc);
    return 1;
  }

  LARGE_INTEGER pc;
  ::QueryPerformanceCounter (& pc);

  if (! ws.model.initialize (pc.QuadPart, ws.width, ws.height)) {
    return 1;
  }

  ::SetTimer (hwnd, IDT_TIMER, 10, nullptr);

  // Enter the main loop.
  MSG msg;
  BOOL bRet;
  while ((bRet = ::GetMessage (& msg, nullptr, 0, 0)) != 0) {
    if (bRet == -1) {
      // TODO: check error code
      return ::GetLastError ();
    }
    else {
      ::TranslateMessage (& msg);
      ::DispatchMessage (& msg);
    }
  }

  return (msg.message == WM_QUIT ? msg.wParam : 1);
}
