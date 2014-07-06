#include "reposition.h"
#include "print.h"

// The reposition_window function repositions the specified window as if it had been created with CW_USEDEFAULT position.

// Step 1. "Create a temporary invisible window with CW_USEDEFAULT as its position and the same height and width as your dialog box."
// Step 2. "See where the window manager puts that temporary window and move your dialog box to match that position."

// Source: Raymond Chen's blog entry of 22 Nov 2013 7:00 AM,
// "How do I get the effect of CW_USEDEFAULT positioning on a window I've already created?"
// http://blogs.msdn.com/b/oldnewthing/archive/2013/11/22/10470631.aspx

#define WC_REPOSWND TEXT ("p")

LRESULT CALLBACK RepositionWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg != WM_NCCREATE) return ::DefWindowProc (hwnd, msg, wParam, lParam);
  // Step 2.
  RECT window_rect;
  ::GetWindowRect (hwnd, & window_rect);
  int x = window_rect.left;
  int y = window_rect.top;
  CREATESTRUCT * cs = (CREATESTRUCT *) lParam;
  HWND other_hwnd = (HWND) cs->lpCreateParams;
  ::SetWindowPos (other_hwnd, HWND_TOP,
                  x, y, 0, 0,
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
  return FALSE; // Abort window creation.
}

void reposition_window (HWND hwnd)
{
  // Step 1.
  RECT window_rect;
  ::GetWindowRect (hwnd, & window_rect);
  int cx = window_rect.right - window_rect.left;
  int cy = window_rect.bottom - window_rect.top;
  HINSTANCE hInstance = (HINSTANCE) ::GetWindowLongPtr (hwnd, GWLP_HINSTANCE);
  WNDCLASS wndclass = { 0, & RepositionWndProc, 0, 0, hInstance, NULL, NULL, NULL, NULL, WC_REPOSWND };
  ::RegisterClass (& wndclass);
  ::CreateWindowEx (0, WC_REPOSWND, TEXT (""), 0,
                    CW_USEDEFAULT, CW_USEDEFAULT, cx, cy,
                    NULL, NULL, hInstance, (LPVOID) hwnd);
  ::UnregisterClass (WC_REPOSWND, hInstance);
}
