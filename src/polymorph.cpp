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

#include "polymorph.h"
#include "aligned-arrays.h"
#include "compiler.h"
#include "glinit.h"
#include "qpc.h"
#include "resources.h"
#include <windowsx.h>

#define WC_MAIN TEXT ("M")

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

// Initial top, left, width and height (not right and bottom).
void get_rect (RECT & rect, const arguments_t & arguments)
{
  if (arguments.mode == parented) {
    ::GetClientRect ((HWND) arguments.numeric_arg, & rect);
    return;
  }
  if constexpr (MONITOR_SELECT_ENABLED) {
    int n = (int) arguments.numeric_arg;
    if (n >= 1) {
      monitor_enum_param_t param = { & rect, n - 1 };
      ::EnumDisplayMonitors (nullptr, nullptr,
        & monitor_enum_proc, (LPARAM) & param);
      if (param.n < 0) {
        rect.right -= rect.left;
        rect.bottom -= rect.top;
        return;
      }
    }
  }
  rect.left   = ::GetSystemMetrics (SM_XVIRTUALSCREEN);
  rect.top    = ::GetSystemMetrics (SM_YVIRTUALSCREEN);
  rect.right  = ::GetSystemMetrics (SM_CXVIRTUALSCREEN);
  rect.bottom = ::GetSystemMetrics (SM_CYVIRTUALSCREEN);
}

ALIGN_STACK
LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Retrieve the window-struct pointer from the window userdata.
  window_struct_t * ws =
    (window_struct_t *) ::GetWindowLongPtr (hwnd, GWLP_USERDATA);

  LRESULT result = 0;
  bool no_defwndproc = true;
  bool close_window = false;

  if (! ws) {
    if (msg == WM_CREATE) {
      result = -1; // Destroy window.
      // Retrieve the window-struct pointer from the window createstruct.
      CREATESTRUCT * cs = (CREATESTRUCT *) lParam;
      ws = (window_struct_t *) cs->lpCreateParams;
      if (ws->hdc = ::GetDC (hwnd); ws->hdc) {
        ws->hglrc = install_rendering_context (ws->hdc);
        if (ws->hglrc) {
          // Successfully installed the rendering context.
          // Store the window-struct pointer in userdata.
          ::SetWindowLongPtr (hwnd, GWLP_USERDATA, LONG_PTR (ws));
          // Once-only model/graphics allocation and initialization.
          // Abort window creation if shader program compilation fails.
          result = ws->model.initialize (qpc ());
        }
      }
    }
    else {
      no_defwndproc = false;
    }
  }
  else {
    run_mode_t mode = ws->arguments.mode;
    switch (msg) {
    case WM_APP: {
      RECT rc;
      get_rect (rc, ws->arguments);

      // Show the window.
      ::SetWindowPos (hwnd, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom,
        SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOCOPYBITS);

      // Remember initial cursor position to detect mouse movement.
      ::GetCursorPos (& ws->initial_cursor_position);
      ::InvalidateRect (hwnd, nullptr, FALSE);

#if TIMING_ENABLED
      ws->frame_counter = 0;
      LARGE_INTEGER freq;
      ::QueryPerformanceFrequency (& freq);
      ::QueryPerformanceCounter (& ws->last_pc);
      ws->pf = 1.0f / (1024 * freq.QuadPart);
#endif

      break;
    }

    case WM_WINDOWPOSCHANGED: {
      WINDOWPOS * wp = (WINDOWPOS *) lParam;
      if (! (wp->flags & SWP_NOSIZE) || wp->flags & SWP_SHOWWINDOW) {
        ws->model.start (wp->cx, wp->cy, ws->settings);
      }
      break;
    }

    case WM_PAINT: {
      ws->model.draw_next ();
      ::SwapBuffers (ws->hdc);
      ::InvalidateRect (hwnd, nullptr, FALSE);
#if TIMING_ENABLED
      ++ ws->frame_counter;
      if (! (ws->frame_counter & 1023)) {
        LARGE_INTEGER pc;
        ::QueryPerformanceCounter (& pc);
        float frame_time = ws->pf * (pc.QuadPart - ws->last_pc.QuadPart);
        ws->last_pc = pc;
        std::cout << std::fixed << std::setprecision (8)
                  << frame_time << " seconds per frame\n";
      }
#endif
      break;
    }

    case WM_SETCURSOR:
      ::SetCursor (mode == screensaver || mode == configure
                     ? nullptr
                     : ::LoadCursor (nullptr, IDC_ARROW));
      break;

    case WM_MOUSEMOVE:
      if (mode == screensaver || mode == configure) {
        // Compare current mouse position with that stored in the window struct.
        DWORD pos = ::GetMessagePos ();
        int dx = GET_X_LPARAM (pos) - ws->initial_cursor_position.x;
        int dy = GET_Y_LPARAM (pos) - ws->initial_cursor_position.y;
        close_window = (dx < -10 || dx > 10) || (dy < -10 || dy > 10);
      }
      break;

    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      close_window = mode != parented;
      break;

    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_NCACTIVATE:
      no_defwndproc = false;
      close_window = (mode == screensaver || mode == configure)
        && LOWORD (wParam) == WA_INACTIVE;
      break;

    case WM_SYSCOMMAND:
      no_defwndproc = (mode == screensaver || mode == configure)
        && wParam == SC_SCREENSAVE;
      break;

    case WM_CLOSE:
      if (mode == configure) {
        if (::GetWindowLongPtr (hwnd, GWL_STYLE) & WS_VISIBLE) {
          // A simple "ShowWindow (SW_HIDE)" suffices to replace SetWindowPos
          // and SetActiveWindow, but fails to actually hide the window if it
          // occupies the full primary monitor rect and the pixel format
          // specifies SWAP_EXCHANGE; now that we use SWAP_COPY, that problem
          // no longer exists, but we still do it this way just in case.
          ::SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW
            | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
          ::SetActiveWindow (ws->hdlg);
        }
      }
      else
        ::DestroyWindow (hwnd);
      break;

    case WM_DESTROY:
      ::wglMakeCurrent (nullptr, nullptr);
      ::wglDeleteContext (ws->hglrc);
      ::PostQuitMessage (0);
      break;

    default:
      no_defwndproc = false;
      break;
    }
  }
  if (close_window) {
    ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  }
  if (! no_defwndproc) {
    result = ::DefWindowProc (hwnd, msg, wParam, lParam);
  }
  return result;
}

HWND create_screensaver_window (window_struct_t & ws)
{
  if (! glinit (ws.hInstance)) return 0;

  LPCTSTR display_name;
  ::LoadString (ws.hInstance, 1, (LPTSTR) (& display_name), 0);

  // Register the window class.
  WNDCLASSEX wndclass = { sizeof (WNDCLASSEX), 0, & MainWndProc, 0, 0,
    ws.hInstance, ws.icon, nullptr, nullptr, nullptr, WC_MAIN, ws.icon_small };
  ATOM wc_main_atom = ::RegisterClassEx (& wndclass);

  // Create the screen saver window.
  HWND hwnd;
  const arguments_t & args = ws.arguments;
  HWND parent = args.mode == parented ? (HWND) args.numeric_arg : nullptr;
  DWORD style = args.mode == parented ? WS_CHILD : WS_POPUP;
  DWORD ex_style =
    (args.mode == screensaver || args.mode == configure ? WS_EX_TOPMOST : 0) |
    (args.mode == persistent ? WS_EX_APPWINDOW : WS_EX_TOOLWINDOW);

  // Retry once on failure (works around sporadic SetPixelFormat
  // failure observed on Intel integrated graphics on Windows 7).
  for (unsigned retries = 0; retries != 2; ++ retries) {
    hwnd = ::CreateWindowEx (ex_style, MAKEINTATOM (wc_main_atom), display_name,
      style, 0, 0, 0, 0, parent, nullptr, ws.hInstance, & ws);
    if (hwnd) {
      break;
    }
    // Window creation failed (window has been destroyed).
  }
  return hwnd;
}
