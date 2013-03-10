#include "config.h"
#include "mswin.h"
#include "glinit.h"
#include "model.h"
#include "random.h"
#include "cmdline.h"
#include "graphics.h"
#include "memory.h"
#include "vector.h"
#include "compiler.h"
#include <windowsx.h>
// #include "tinyscheme-config.h"
// #include <scheme-private.h>
// #include <scheme.h>
#include <cstdint>

inline std::uint64_t qpc ()
{
  LARGE_INTEGER t;
  ::QueryPerformanceCounter (& t);
  return t.QuadPart;
}

inline bool dispatch_queued_messages (MSG & msg)
{
  bool quit;
  while ((quit = ::PeekMessage (& msg, nullptr, 0, 0, PM_REMOVE)) && msg.message != WM_QUIT) {
    ::TranslateMessage (& msg);
    ::DispatchMessage (& msg);
  }
  return ! quit;
}

inline int main_loop (HDC hdc, model_t & model)
{
  MSG msg;
  while (dispatch_queued_messages (msg)) {
    model.proceed ();
    ::SwapBuffers (hdc);
    clear ();
    model.draw ();
  }
  return static_cast <int> (msg.wParam);
}

struct window_info_t
{
  HDC hdc;
  int width, height;
};

window_info_t * create_screensaver_window (HINSTANCE hInstance, HWND parent, run_mode_t mode);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, int)
{
  HWND parent = nullptr;
  run_mode_t mode = parse_command_line (lpszCmdLine, & parent);

  if (mode == configure) {
    ::MessageBox (parent, usr::message, usr::message_window_name, MB_OK | MB_ICONASTERISK);
    return 0;
  }

  if (window_info_t * wi = create_screensaver_window (hInstance, parent, mode)) {
    model_t model ALIGNED16;
    unsigned seed = qpc ();
    if (model.initialize (seed, wi->width, wi->height)) {
      return main_loop (wi->hdc, model);
    }
  }
  return 1;
}

struct create_params_t
{
  run_mode_t mode;
};

struct window_struct_t
{
  window_info_t info;
  HGLRC hglrc;
  POINT initial_cursor_position;
  run_mode_t mode;
};

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  bool call_def_window_proc = false, close_window = false;

  window_struct_t * ws = reinterpret_cast <window_struct_t *> (::GetWindowLongPtr (hwnd, GWLP_USERDATA));

  switch (msg)
  {
  case WM_CREATE:
    result = -1;
    if ((ws = reinterpret_cast <window_struct_t *> (allocate_internal (sizeof (window_struct_t))))) {
      ::SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (ws));
      CREATESTRUCT * cs = reinterpret_cast <CREATESTRUCT *> (lParam);
      create_params_t * cp = reinterpret_cast <create_params_t *> (cs->lpCreateParams);
      ws->mode = cp->mode;
      ::GetCursorPos (& ws->initial_cursor_position);
      if ((ws->info.hdc = ::GetDC (hwnd)) && ((ws->hglrc = initialize_opengl (cs->hInstance, ws->info.hdc)))) {
        result = 0;
      }
    }
    break;

  case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS * wp = reinterpret_cast <WINDOWPOS *> (lParam);
      ws->info.width = wp->cx;
      ws->info.height = wp->cy;
      break;
    }

  case WM_SETCURSOR:
    ::SetCursor (ws->mode == fullscreen ? nullptr : (::LoadCursor (nullptr, IDC_ARROW)));
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
    call_def_window_proc = (ws->mode != fullscreen || wParam != SC_SCREENSAVE) && wParam != SC_CLOSE;
    break;

  case WM_DESTROY:
    ::wglMakeCurrent (nullptr, nullptr);
    ::wglDeleteContext (ws->hglrc);
    ::ReleaseDC (hwnd, ws->info.hdc);
    ::SetWindowLongPtr (hwnd, GWLP_USERDATA, 0);
    deallocate (ws);
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

window_info_t * create_screensaver_window (HINSTANCE hInstance, HWND parent, run_mode_t mode)
{
  DWORD style;
  DWORD ex_style;
  int left, top, width, height;
  create_params_t cp = { mode };

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
  wc.hInstance = hInstance;
  wc.hIcon = ::LoadIcon (hInstance, MAKEINTRESOURCE (257));
  wc.lpszClassName = usr::window_class_name;

  ATOM atom;
  HWND hwnd;
  if ((atom = ::RegisterClass (& wc)) &&               // HWND CreateWindowEx
      (hwnd = ::CreateWindowEx (ex_style,              //   (DWORD dwExStyle,
                                MAKEINTATOM (atom),    //    LPCTSTR lpClassName,
                                usr::window_name,      //    LPCTSTR lpWindowName,
                                style,                 //    DWORD dwStyle,
                                left,                  //    int x,
                                top,                   //    int y,
                                width,                 //    int nWidth,
                                height,                //    int nHeight,
                                parent,                //    HWND hWndParent,
                                nullptr,               //    HMENU hMenu,
                                hInstance,             //    HINSTANCE hInstance,
                                & cp))) {              //    LPVOID lpParam);
    return reinterpret_cast <window_info_t *> (::GetWindowLongPtr (hwnd, GWLP_USERDATA));
  }
  return nullptr;
}
