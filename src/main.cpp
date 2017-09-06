// Copyright 2012-2017 Richard Copley
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
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "arguments.h"
#include "polymorph.h"
#include "settings.h"
#include "dialog.h"
#include "qpc.h"
#include "vector.h"
#include <cstdint>
#include <cstring>

#define WC_MAIN TEXT ("M")

int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  _mm_setcsr (_mm_getcsr () | (_MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON));

  if (! glinit (hInstance)) return 1;

  INITCOMMONCONTROLSEX icc = { sizeof (INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_LINK_CLASS | ICC_STANDARD_CLASSES };
  ::InitCommonControlsEx (& icc);

  LPCTSTR display_name;
  ::LoadString (hInstance, 1, (LPTSTR) (& display_name), 0);

  ALIGNED16 window_struct_t ws {};

  get_arguments (::GetCommandLine (), ws.arguments);
  load_settings (ws.settings);

  // Register screen saver window class.

  // The screen saver window has no taskbar button or system menu, but its small icon
  // is inherited by the (owned) configure dialog and used for /its/ taskbar button.
  // We could set the dialog's small icon directly, but then it would appear in the
  // dialog's system menu, which is not the desired effect.
  HICON icon = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 0, 0, LR_LOADTRANSPARENT);
  HICON icon_small = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
  WNDCLASSEX wndclass = { sizeof (WNDCLASSEX), 0, & MainWndProc, 0, 0, hInstance, icon, NULL, NULL, NULL, WC_MAIN, icon_small };
  ATOM wc_main_atom = ::RegisterClassEx (& wndclass);

  // Create the screen saver window.
  HWND hwnd;
  HWND parent = ws.arguments.mode == parented ? (HWND) ws.arguments.numeric_arg : NULL;
  DWORD style = ws.arguments.mode == parented ? WS_CHILD : WS_POPUP;
  DWORD ex_style =
    (ws.arguments.mode == screensaver || ws.arguments.mode == configure ? WS_EX_TOPMOST : 0) |
    (ws.arguments.mode == persistent ? WS_EX_APPWINDOW : WS_EX_TOOLWINDOW);

  // Retry once on failure (works around sporadic SetPixelFormat
  // failure observed on Intel integrated graphics on Windows 7).
  for (unsigned retries = 0; retries != 2; ++ retries) {
    hwnd = ::CreateWindowEx (ex_style, MAKEINTATOM (wc_main_atom), display_name, style,
                             0, 0, 0, 0,
                             parent, NULL, hInstance, & ws);
    if (hwnd) break;
    // Window creation failed (window will be destroyed).
  }

  if (! hwnd) return 1;

  // Window creation succeeded. Initialize the simulation.
  if (! ws.model.initialize (qpc ())) return 1;

  // Create the configure dialog if in configure mode.
  dialog_struct_t ds = { ws.settings, hwnd, };

  // Show the main window, or the configure dialog if in configure mode.
  if (ws.arguments.mode == configure) {
    ws.hdlg = ::CreateDialogParam (hInstance, MAKEINTRESOURCE (IDD_CONFIGURE), ds.hwnd, DialogProc, (LPARAM) & ds);
  }
  else {
    ::PostMessage (hwnd, WM_APP, 0, 0);
  }
  MSG msg;
  while (::GetMessage (& msg, NULL, 0, 0)) {
    if (! ws.hdlg || ! ::IsDialogMessage (ws.hdlg, & msg)) {
      ::TranslateMessage (& msg);
      ::DispatchMessage (& msg);
    }
  }
  return (int) msg.wParam;
}

#ifdef TINY

// Tiny startup.

// No standard handles, window placement, environment variables,
// command-line transformation, global constructors and destructors,
// atexit functions, thread-local storage, runtime relocation fixups,
// 387 floating-point initialization, signal handlers and no exceptions.
extern "C"
{
  // This symbol is defined by the linker.
  extern IMAGE_DOS_HEADER __ImageBase;

  // In the linker command line, specify this function as the entry point.
  VISIBLE NORETURN ALIGN_STACK DWORD CALLBACK RawEntryPoint ()
  {
    HINSTANCE hInstance = (HINSTANCE) & __ImageBase;
    int status = _tWinMain (hInstance, NULL, NULL, 0);
    ::ExitProcess ((UINT) (status));
  }
  DWORD _RawEntryPoint () __attribute__ ((weak, alias("RawEntryPoint")));
}

#endif
