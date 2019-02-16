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

#include "mswin.h"
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
  WNDCLASSEX wndclass = { sizeof (WNDCLASSEX), 0, & RepositionWndProc, 0, 0, hInstance, NULL, NULL, NULL, NULL, WC_REPOSWND, NULL };
  ::RegisterClassEx (& wndclass);
  ::CreateWindowEx (0, WC_REPOSWND, TEXT (""), 0,
                    CW_USEDEFAULT, CW_USEDEFAULT, cx, cy,
                    NULL, NULL, hInstance, (LPVOID) hwnd);
  ::UnregisterClass (WC_REPOSWND, hInstance);
}
