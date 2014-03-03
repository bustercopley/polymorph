#include "compiler.h"
#include "mswin.h"
#include "glinit.h"
#include "model.h"
#include "cmdline.h"
#include <windowsx.h>
// #include "tinyscheme-config.h"
// #include <scheme-private.h>
// #include <scheme.h>
#include <cstdint>

namespace usr {
  // Program name.
  static const TCHAR * const program_name = TEXT ("Polymorph");
  static const TCHAR * const message =
  TEXT ("And the ratios of their numbers, motions, and ")
  TEXT ("other properties, everywhere God, as far as ")
  TEXT ("necessity allowed or gave consent, has exactly ")
  TEXT ("perfected, and harmonised in due proportion.");
  // TEXT ("\n\nThis screensaver includes TinyScheme, developed by Dimitrios ")
  // TEXT ("Souflis and licensed under the Modified BSD License. ")
  // TEXT ("See \"tinyscheme/COPYING.txt\" for details.");
}

inline std::uint64_t qpc ()
{
  LARGE_INTEGER t;
  ::QueryPerformanceCounter (& t);
  return t.QuadPart;
}

struct window_struct_t
{
  model_t model;
  POINT initial_cursor_position;
  run_mode_t mode;
};

LRESULT CALLBACK InitWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HGLRC setup_opengl_context (HWND hwnd);

// Called by custom_entry_point (below).
int custom_main (HINSTANCE hInstance)
{
  // Read command line arguments.

  HWND parent = NULL;
  run_mode_t mode = parse_command_line (::GetCommandLine (), & parent);

  if (mode == configure) {
    ::MessageBox (NULL, usr::message, usr::program_name, MB_OK | MB_ICONASTERISK);
    return 0;
  }

  // Placement and window style of the main window.

  RECT rect;
  DWORD style;
  DWORD ex_style  = 0;

  if (mode == embedded) {
    style = WS_CHILD | WS_VISIBLE;
    ::GetClientRect (parent, & rect); // 0, 0, width, height
  }
  else {
    style = WS_POPUP | WS_VISIBLE;
    if (mode == fullscreen) ex_style = WS_EX_TOPMOST;
    rect.left =   ::GetSystemMetrics (SM_XVIRTUALSCREEN);
    rect.top =    ::GetSystemMetrics (SM_YVIRTUALSCREEN);
    rect.right =  ::GetSystemMetrics (SM_CXVIRTUALSCREEN); // actually width, not right
    rect.bottom = ::GetSystemMetrics (SM_CYVIRTUALSCREEN); // actually height, not bottom
  }

  window_struct_t ws ALIGNED16;
  ws.mode = mode;

  // Create a window with an OpenGL rendering context.

  // To obtain a proper OpenGL pixel format, we need to call wglChoosePixelFormatARB, but
  // first we must obtain the address of that function by calling using wglGetProcAddress,
  // which requires that an OpenGL rendering context is current, which requires a device
  // context that supports OpenGL, which requires a window with an OpenGL pixel format.

  // Bootstrap the process with a legacy OpenGL pixel format. According to MSDN,
  // "Once a window's pixel format is set, it cannot be changed", so the window
  // and associated resources are of no further use and are destroyed here.

  // Create the dummy window. See InitWndProc.
  // Note this window does not survive creation.
  WNDCLASS init_wc;
  ::ZeroMemory (& init_wc, sizeof init_wc);
  init_wc.hInstance = hInstance;
  init_wc.lpfnWndProc = & InitWndProc;
  init_wc.lpszClassName = TEXT ("GLinit");
  ATOM init_wc_atom = ::RegisterClass (& init_wc);
  ::CreateWindowEx (0, MAKEINTATOM (init_wc_atom), TEXT (""), 0,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, hInstance, NULL);
  ::UnregisterClass (MAKEINTATOM (init_wc_atom), hInstance);

  // Exit if we failed to get the function pointers.
  if (! wglChoosePixelFormatARB) return -1;

  // Create the main window. See MainWndProc.
  WNDCLASS main_wc;
  ::ZeroMemory (& main_wc, sizeof main_wc);
  main_wc.hInstance = hInstance;
  main_wc.lpfnWndProc = & MainWndProc;
  main_wc.hIcon = ::LoadIcon (hInstance, MAKEINTRESOURCE (257));
  main_wc.lpszClassName = usr::program_name;
  ATOM main_wc_atom = ::RegisterClass (& main_wc);
  HWND hwnd = ::CreateWindowEx (ex_style, MAKEINTATOM (main_wc_atom), usr::program_name, style,
                                rect.left, rect.top, rect.right, rect.bottom,
                                parent, NULL, hInstance, & ws);

  // Exit if we failed to create the window.
  if (! hwnd) return 1;

  // Enter the main loop.
  MSG msg;
  while (::GetMessage (& msg, NULL, 0, 0)) {
    ::TranslateMessage (& msg);
    ::DispatchMessage (& msg);
  }

  ::UnregisterClass (MAKEINTATOM (main_wc_atom), hInstance);
  return msg.wParam;
}

LRESULT CALLBACK InitWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_NCCREATE) {
    // Set up a legacy rendering context to get the OpenGL function pointers.
    PIXELFORMATDESCRIPTOR pfd;
    ::ZeroMemory (& pfd, sizeof pfd);
    pfd.nSize = sizeof pfd;
    pfd.dwFlags = PFD_SUPPORT_OPENGL;
    if (HDC hdc = ::GetDC (hwnd)) {
      int pf = ::ChoosePixelFormat (hdc, & pfd);
      ::SetPixelFormat (hdc, pf, & pfd);
      if (HGLRC hglrc = ::wglCreateContext (hdc)) {
        ::wglMakeCurrent (hdc, hglrc);
        // Get the function pointers.
        get_glprocs ();
        ::wglMakeCurrent (NULL, NULL);
        ::wglDeleteContext (hglrc);
      }
      ::ReleaseDC (hwnd, hdc);
    }
    return FALSE; // Abort window creation.
  }
  else {
    return ::DefWindowProc (hwnd, msg, wParam, lParam);
  }
}

LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  bool call_def_window_proc = false, close_window = false;

  // Retrieve the window-struct pointer from the window userdata.
  window_struct_t * ws = reinterpret_cast <window_struct_t *> (::GetWindowLongPtr (hwnd, GWLP_USERDATA));

  switch (msg) {
  case WM_CREATE: {
    result = -1; // Abort window creation.

    // Stash the window-struct pointer in the window userdata.
    CREATESTRUCT * cs = reinterpret_cast <CREATESTRUCT *> (lParam);
    ws = reinterpret_cast <window_struct_t *> (cs->lpCreateParams);
    ::SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (ws));

    // Remember initial mouse-pointer position to detect mouse movement.
    ::GetCursorPos (& ws->initial_cursor_position);

    // Set up OpenGL rendering context.
    HGLRC hglrc = setup_opengl_context (hwnd);
    if (hglrc) {
      ws->model.initialize (qpc (), cs->cx, cs->cy);
      ::PostMessage (hwnd, WM_APP, 0, 0);  // Start the simulation.
      result = 0;                          // Allow window creation to continue.
    }
    break;
  }

  case WM_APP: {
    ws->model.draw_next ();
    ::InvalidateRect (hwnd, NULL, FALSE);
    break;
  }

  case WM_PAINT: {
    PAINTSTRUCT ps;
    ::BeginPaint (hwnd, & ps);
    ::SwapBuffers (ps.hdc);
    ::EndPaint (hwnd, & ps);
    ::PostMessage (hwnd, WM_APP, 0, 0);
    break;
  }

  case WM_SETCURSOR:
    ::SetCursor (ws->mode == fullscreen ? NULL : (::LoadCursor (NULL, IDC_ARROW)));
    break;

  case WM_MOUSEMOVE:
    if (ws->mode == fullscreen) {
      // Compare the current mouse position with the one stored in the window struct.
      POINT current = { GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) };
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
    call_def_window_proc = ! (ws->mode == fullscreen && wParam == SC_SCREENSAVE);
    break;

  case WM_DESTROY:
    ::wglMakeCurrent (NULL, NULL);
    //::wglDeleteContext (ws->hglrc);
    ::PostQuitMessage (0);
    break;

  default:
    call_def_window_proc = true;
    break;
  }

  if (close_window) {
    ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  }

  if (call_def_window_proc) {
    result = ::DefWindowProc (hwnd, msg, wParam, lParam);
  }

  return result;
}

// Set up a proper pixel format and rendering context for the main window.
HGLRC setup_opengl_context (HWND hwnd)
{
  const int pf_attribs [] = {
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

  const int context_attribs [] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 2,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0, 0,
  };

  HGLRC hglrc = NULL;
  int pf;
  UINT pfcount;
  if (HDC hdc = ::GetDC (hwnd)) {
    if (wglChoosePixelFormatARB (hdc, pf_attribs, NULL, 1, & pf, & pfcount)) {
      if (::SetPixelFormat (hdc, pf, NULL)) {
        hglrc = wglCreateContextAttribsARB (hdc, NULL, context_attribs);
        if (hglrc) {
          ::wglMakeCurrent (hdc, hglrc);
        }
      }
    }
    ::ReleaseDC (hwnd, hdc);
  }
  return hglrc;
}

// Tiny startup.

// No standard handles, window placement, environment variables,
// command-line transformation, global constructors and destructors,
// atexit functions, stack realignment, thread-local storage,
// runtime relocation fixups, 387 floating-point initialization,
// signal handlers or exceptions.

extern "C"
{
  // This symbol is provided by all recent GCC and MSVC linkers.
  extern IMAGE_DOS_HEADER __ImageBase;

  // This entry point must be specified in the linker command line.
  extern void custom_startup ()
  {
    HINSTANCE hInstance = reinterpret_cast <HINSTANCE> (& __ImageBase);
    int status = custom_main (hInstance);
    ::ExitProcess (static_cast <UINT> (status));
  }
}
