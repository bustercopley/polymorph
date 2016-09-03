#include "mswin.h"
#include "compiler.h"
#include <windowsx.h>
#include "polymorph.h"
#include "resources.h"
#include "glinit.h"
#include "aligned-arrays.h"
#include "print.h"

#if MONITOR_SELECT_ENABLED

struct monitor_enum_param_t
{
  RECT * r;
  int n;
};

BOOL CALLBACK monitor_enum_proc (HMONITOR, HDC, LPRECT rect, LPARAM lparam)
{
  monitor_enum_param_t * pparam = (monitor_enum_param_t *) lparam;
  if (pparam->n == 0) * pparam->r = * rect;
  -- pparam->n;
  return TRUE;
}

#endif

bool get_rect (RECT & rect, const arguments_t & arguments)
{
  if (arguments.mode == parented) {
    ::GetClientRect ((HWND) arguments.numeric_arg, & rect);
    return true;
  }
#if MONITOR_SELECT_ENABLED
  int n = (int) arguments.numeric_arg;
  if (n >= 1) {
    monitor_enum_param_t param = { & rect, n - 1 };
    ::EnumDisplayMonitors (NULL, NULL, & monitor_enum_proc, (LPARAM) & param);
    if (param.n < 0) return true;
  }
#endif
  return false;
}

ALIGN_STACK LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Retrieve the window-struct pointer from the window userdata.
  window_struct_t * ws = (window_struct_t *) ::GetWindowLongPtr (hwnd, GWLP_USERDATA);

  LRESULT result = 0;
  bool call_def_window_proc = false;
  bool close_window = false;

  if (! ws) {
    if (msg == WM_CREATE) {
      result = -1; // Destroy window.
      // Retrieve the window-struct pointer from the window createsruct.
      CREATESTRUCT * cs = (CREATESTRUCT *) lParam;
      ws = (window_struct_t *) cs->lpCreateParams;
      if (HDC hdc = ::GetDC (hwnd)) {
        ws->hglrc = install_rendering_context (hdc);
        if (ws->hglrc) {
          // Context installed successfully. Store the window-struct pointer in userdata.
          ::SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (ws));
          result = 0; // Continue window creation.
        }
        ::ReleaseDC (hwnd, hdc);
      }
    }
    else {
      call_def_window_proc = true;
    }
  }
  else {
    run_mode_t mode = ws->arguments.mode;
    switch (msg) {
    case WM_WINDOWPOSCHANGING: {
      WINDOWPOS * windowpos = (WINDOWPOS *) lParam;
      if (windowpos->flags & SWP_SHOWWINDOW) {
        windowpos->flags &= ~ (SWP_NOSIZE | SWP_NOMOVE);

        RECT rect;
        if (get_rect (rect, ws->arguments)) {
          windowpos->x  = rect.left;
          windowpos->y  = rect.top;
          windowpos->cx = rect.right - rect.left;
          windowpos->cy = rect.bottom - rect.top;
        }
        else {
          windowpos->x  = ::GetSystemMetrics (SM_XVIRTUALSCREEN);
          windowpos->y  = ::GetSystemMetrics (SM_YVIRTUALSCREEN);
          windowpos->cx = ::GetSystemMetrics (SM_CXVIRTUALSCREEN);
          windowpos->cy = ::GetSystemMetrics (SM_CYVIRTUALSCREEN);
        }
      }
      break;
    }

    case WM_WINDOWPOSCHANGED: {
      WINDOWPOS * windowpos = (WINDOWPOS *) lParam;
      if (windowpos->flags & SWP_SHOWWINDOW) {
        // Remember initial cursor position to detect mouse movement.
        ::GetCursorPos (& ws->initial_cursor_position);
        // (Re-)start the simulation.
        ws->model.start (windowpos->cx, windowpos->cy, ws->settings);
        ::PostMessage (hwnd, WM_APP, 0, 0);
      }
      break;
    }

    case WM_APP:
      ws->model.draw_next ();
      ::InvalidateRect (hwnd, NULL, FALSE);
      break;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      ::BeginPaint (hwnd, & ps);
      ::SwapBuffers (ps.hdc);
      ::EndPaint (hwnd, & ps);
      ::PostMessage (hwnd, WM_APP, 0, 0);
      break;
    }

    case WM_SETCURSOR:
      ::SetCursor (mode == screensaver || mode == configure ? NULL : ::LoadCursor (NULL, IDC_ARROW));
      break;

    case WM_MOUSEMOVE:
      if (mode == screensaver || mode == configure) {
        // Compare the current mouse position with the one stored in the window struct.
        DWORD pos = ::GetMessagePos ();
        int dx = GET_X_LPARAM (pos) - ws->initial_cursor_position.x;
        int dy = GET_Y_LPARAM (pos) - ws->initial_cursor_position.y;
        close_window = (dx < -10 || dx > 10) || (dy < -10 || dy > 10);
      }
      break;

    case WM_KEYDOWN: case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
      close_window = mode != parented;
      break;

    case WM_ACTIVATE: case WM_ACTIVATEAPP: case WM_NCACTIVATE:
      close_window = (mode == screensaver || mode == configure) && LOWORD (wParam) == WA_INACTIVE;
      call_def_window_proc = true;
      break;

    case WM_SYSCOMMAND:
      call_def_window_proc = ! ((mode == screensaver || mode == configure) && wParam == SC_SCREENSAVE);
      break;

    case WM_CLOSE:
      if (mode == configure) {
        if (::GetWindowLongPtr (hwnd, GWL_STYLE) & WS_VISIBLE) {
          // Workaround for bug observed on Windows 8.1 where hiding
          // a full-monitor OpenGL window does not remove it from the display:
          // resize the window before hiding it.
          ::SetWindowPos (hwnd, NULL, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOCOPYBITS);
          ::ShowWindow (hwnd, SW_HIDE);
        }
      }
      else
        ::DestroyWindow (hwnd);
      break;

    case WM_DESTROY:
      ::wglMakeCurrent (NULL, NULL);
      ::wglDeleteContext (ws->hglrc);
      ::PostQuitMessage (0);
      break;

    default:
      call_def_window_proc = true;
      break;
    }
  }
  if (close_window) ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  if (call_def_window_proc) result = ::DefWindowProc (hwnd, msg, wParam, lParam);
  return result;
}
