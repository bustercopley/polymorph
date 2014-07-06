#include "compiler.h"
#include "mswin.h"
#include <windowsx.h>
#include "polymorph.h"
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "aligned-arrays.h"
#include "qpc.h"
#include "print.h"

#define WC_MAIN TEXT ("M")

ALIGN_STACK LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  bool call_def_window_proc = false, close_window = false;

  // Retrieve the window-struct pointer from the window userdata.
  window_struct_t * ws = reinterpret_cast <window_struct_t *> (::GetWindowLongPtr (hwnd, GWLP_USERDATA));

  switch (msg) {
  case WM_CREATE: {
    result = -1; // Abort window creation.

    // Store the window-struct pointer in the window userdata.
    CREATESTRUCT * cs = (CREATESTRUCT *) lParam;
    ws = (window_struct_t *) cs->lpCreateParams;
    ::SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (ws));

    ws->hglrc = install_rendering_context (hwnd);
    if (ws->hglrc) {
      ws->model.initialize (qpc ());
      result = 0; // Allow window creation to continue.
    }
    break;
  }

  case WM_WINDOWPOSCHANGING: {
    WINDOWPOS * windowpos = (WINDOWPOS *) lParam;
    if (windowpos->flags & SWP_SHOWWINDOW) {
      if (ws->mode == parented) {
        RECT rect;
        ::GetClientRect (::GetParent (hwnd), & rect);
        windowpos->cx = rect.right;
        windowpos->cy = rect.bottom;
      }
      else {
        windowpos->x  = ::GetSystemMetrics (SM_XVIRTUALSCREEN);
        windowpos->y  = ::GetSystemMetrics (SM_YVIRTUALSCREEN);
        windowpos->cx = ::GetSystemMetrics (SM_CXVIRTUALSCREEN);
        windowpos->cy = ::GetSystemMetrics (SM_CYVIRTUALSCREEN);
      }
      windowpos->flags &= ~ (SWP_NOSIZE | SWP_NOMOVE);
    }
    break;
  }

  case WM_WINDOWPOSCHANGED: {
    WINDOWPOS * windowpos = (WINDOWPOS *) lParam;
    if (windowpos->flags & SWP_SHOWWINDOW) {
      // Remember initial cursor position to detect mouse movement.
      ::GetCursorPos (& ws->initial_cursor_position);
      // (Re-)start the simulation.
      ws->model.start (windowpos->cx, windowpos->cy, * ws->settings);
      ws->model.draw_next ();
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
    ::SetCursor (ws->mode == screensaver || ws->mode == configure ? NULL : ::LoadCursor (NULL, IDC_ARROW));
    break;

  case WM_MOUSEMOVE:
    if (ws->mode == screensaver || ws->mode == configure) {
      // Compare the current mouse position with the one stored in the window struct.
      DWORD pos = ::GetMessagePos ();
      int dx = GET_X_LPARAM (pos) - ws->initial_cursor_position.x;
      int dy = GET_Y_LPARAM (pos) - ws->initial_cursor_position.y;
      close_window = (dx < -10 || dx > 10) || (dy < -10 || dy > 10);
    }
    break;

  case WM_KEYDOWN: case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
    close_window = ws->mode != parented;
    break;

  case WM_ACTIVATE: case WM_ACTIVATEAPP: case WM_NCACTIVATE:
    close_window = (ws->mode == screensaver || ws->mode == configure) && LOWORD (wParam) == WA_INACTIVE;
    call_def_window_proc = true;
    break;

  case WM_SYSCOMMAND:
    call_def_window_proc = ! ((ws->mode == screensaver || ws->mode == configure) && wParam == SC_SCREENSAVE);
    break;

  case WM_CLOSE:
    if (ws->mode == configure) {
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
    if (ws->hglrc) ::wglDeleteContext (ws->hglrc);
    ::PostQuitMessage (0);
    break;

  default:
    call_def_window_proc = true;
    break;
  }

  if (close_window) ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  if (call_def_window_proc) result = ::DefWindowProc (hwnd, msg, wParam, lParam);

  return result;
}

void register_class (HINSTANCE hInstance)
{
  // The screensaver window has no taskbar button or system menu, but the small
  // icon is inherited by the (owned) configure dialog and used for the taskbar button.
  // We could set the dialog's small icon directly, but then it would appear in
  // the dialog's system menu, which is not the desired effect.
  HICON icon = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
  HICON icon_small = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
  WNDCLASSEX wndclass = { sizeof (WNDCLASSEX), 0, & MainWndProc, 0, 0, hInstance, icon, NULL, NULL, NULL, WC_MAIN, icon_small };
  ::RegisterClassEx (& wndclass);
}

HWND create_window (HINSTANCE hInstance, HWND parent, LPCTSTR display_name, window_struct_t * ws)
{
  // Create the main window. See MainWndProc for details.
  DWORD style = ws->mode == parented ? WS_CHILD : WS_POPUP;
  DWORD ex_style = (ws->mode == screensaver || ws->mode == configure ? WS_EX_TOPMOST : 0) | WS_EX_TOOLWINDOW;
  return ::CreateWindowEx (ex_style, WC_MAIN, display_name, style,
                           0, 0, 0, 0,
                           parent, NULL, hInstance, ws);
}
