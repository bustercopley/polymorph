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
#include "compiler.h"
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "arguments.h"
#include "polymorph.h"
#include "settings.h"
#include "dialog.h"
#include "vector.h"
#include <cstdint>
#include <cstring>

int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  _mm_setcsr (_mm_getcsr () | (_MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON));

  INITCOMMONCONTROLSEX icc = { sizeof (INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_LINK_CLASS | ICC_STANDARD_CLASSES };
  ::InitCommonControlsEx (& icc);

  ALIGNED16 window_struct_t ws {};

  ws.hInstance = hInstance;
  ::LoadString (hInstance, 1, (LPTSTR) (& ws.display_name), 0);
  ws.icon = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
  ws.icon_small = (HICON) ::LoadImage (hInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 16, 16, 0);
  get_arguments (::GetCommandLine (), ws.arguments);
  load_settings (ws.settings);

  // Show the main window, or the configure dialog if in configure mode.
  if (ws.arguments.mode == configure) {
    ws.hdlg = ::CreateDialogParam (hInstance, MAKEINTRESOURCE (IDD_CONFIGURE), NULL, DialogProc, (LPARAM) & ws);
  }
  else {
    HWND hwnd = create_screensaver_window (ws);
    if (! hwnd) return 1;
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
}

#endif
