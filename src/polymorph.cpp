// Copyright 2016 Richard Copley
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

// Initial top, left, width and height (not right and bottom).
void get_rect (RECT & rect, const arguments_t & arguments)
{
  if (arguments.mode == parented) {
    ::GetClientRect ((HWND) arguments.numeric_arg, & rect);
    return;
  }
#if MONITOR_SELECT_ENABLED
  int n = (int) arguments.numeric_arg;
  if (n >= 1) {
    monitor_enum_param_t param = { & rect, n - 1 };
    ::EnumDisplayMonitors (NULL, NULL, & monitor_enum_proc, (LPARAM) & param);
    if (param.n < 0) {
      rect.right -= rect.left;
      rect.bottom -= rect.top;
      return;
    }
  }
#endif
  rect.left   = ::GetSystemMetrics (SM_XVIRTUALSCREEN);
  rect.top    = ::GetSystemMetrics (SM_YVIRTUALSCREEN);
  rect.right  = ::GetSystemMetrics (SM_CXVIRTUALSCREEN);
  rect.bottom = ::GetSystemMetrics (SM_CYVIRTUALSCREEN);
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
    case WM_APP: {
      RECT rect;
      get_rect (rect, ws->arguments);

      // Show the window.
      ::SetWindowPos (hwnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom,
        SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOCOPYBITS);

      // Remember initial cursor position to detect mouse movement.
      ::GetCursorPos (& ws->initial_cursor_position);
      ::InvalidateRect (hwnd, nullptr, FALSE);
      break;
    }

    case WM_WINDOWPOSCHANGED: {
      WINDOWPOS * windowpos = (WINDOWPOS *) lParam;
      if (! (windowpos->flags & SWP_NOSIZE) || windowpos->flags & SWP_SHOWWINDOW) {
        ws->model.start (windowpos->cx, windowpos->cy, ws->settings);
      }
      break;
    }

    case WM_PAINT: {
      ws->model.draw_next ();
      PAINTSTRUCT ps;
      ::BeginPaint (hwnd, & ps);
      ::SwapBuffers (ps.hdc);
      ::EndPaint (hwnd, & ps);
      ::InvalidateRect (hwnd, nullptr, FALSE);
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
          // A simple "ShowWindow (SW_HIDE)" suffices to replace SetWindowPos and SetActiveWindow,
          // but fails to actually hide the window if it occupies the full primary monitor rect
          // and the pixel format specifies SWAP_EXCHANGE; now that we use SWAP_COPY, that problem
          // no longer exists, but we still do it this way just in case.
          ::SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0,
            SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
          ::SetActiveWindow (ws->hdlg);
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
  if (close_window) {
    ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  }
  if (call_def_window_proc) result = ::DefWindowProc (hwnd, msg, wParam, lParam);
  return result;
}
