#include "compiler.h"
#include "mswin.h"
#include <windowsx.h>
#include "polymorph.h"
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "aligned-arrays.h"
#include "qpc.h"
#include <new>

LRESULT CALLBACK MainWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

    // Remember initial mouse-pointer position to detect mouse movement.
    POINT cursor;
    ::GetCursorPos (& cursor);
    ::ScreenToClient (hwnd, & cursor);
    ws->initial_cursor_position = cursor;

    ws->hglrc = install_rendering_context (hwnd);
    if (ws->hglrc) {
      LARGE_INTEGER qpc;
      ::QueryPerformanceCounter (& qpc);
      ws->model.initialize (qpc.QuadPart, cs->cx, cs->cy);
      result = 0; // Allow window creation to continue.
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
    ::SetCursor (ws->mode == screensaver ? NULL : ::LoadCursor (NULL, IDC_ARROW));
    break;

  case WM_MOUSEMOVE:
    if (ws->mode == screensaver) {
      // Compare the current mouse position with the one stored in the window struct.
      DWORD cursor = (DWORD) lParam;
      int dx = GET_X_LPARAM (cursor) - ws->initial_cursor_position.x;
      int dy = GET_Y_LPARAM (cursor) - ws->initial_cursor_position.y;
      close_window = (dx < -10 || dx > 10) || (dy < -10 || dy > 10);
    }
    break;

  case WM_KEYDOWN: case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
    close_window = ws->mode != parented;
    break;

  case WM_ACTIVATE: case WM_ACTIVATEAPP: case WM_NCACTIVATE:
    close_window = ws->mode == screensaver && LOWORD (wParam) == WA_INACTIVE;
    call_def_window_proc = true;
    break;

  case WM_SYSCOMMAND:
    call_def_window_proc = ! (ws->mode == screensaver && wParam == SC_SCREENSAVE);
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

ATOM register_class (HINSTANCE hInstance)
{
  HICON icon = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
  WNDCLASS wndclass = { 0, & MainWndProc, 0, 0, hInstance, icon, NULL, NULL, NULL, TEXT ("Polymorph") };
  ATOM wndclass_id = ::RegisterClass (& wndclass);
  ::DestroyIcon (icon);
  return wndclass_id;
}

HWND create_window (HINSTANCE hInstance, HWND parent, ATOM wndclass_id, LPCTSTR display_name, window_struct_t * ws)
{
  // Create the main window. See MainWndProc for details.
  DWORD style = ws->mode == parented ? WS_CHILD : WS_POPUP;
  DWORD ex_style = ws->mode == screensaver ? WS_EX_TOPMOST : 0;
  RECT rect;
  if (ws->mode == parented) {
    ::GetClientRect (parent, & rect);
  }
  else {
    rect.left =   ::GetSystemMetrics (SM_XVIRTUALSCREEN);
    rect.top =    ::GetSystemMetrics (SM_YVIRTUALSCREEN);
    rect.right =  ::GetSystemMetrics (SM_CXVIRTUALSCREEN); // actually width, not right
    rect.bottom = ::GetSystemMetrics (SM_CYVIRTUALSCREEN); // actually height, not bottom
  }
  return ::CreateWindowEx (ex_style, MAKEINTATOM (wndclass_id), display_name, style,
                           rect.left, rect.top, rect.right, rect.bottom,
                           parent, NULL, hInstance, ws);
}
