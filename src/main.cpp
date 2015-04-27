#include "compiler.h"
#include "mswin.h"
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "arguments.h"
#include "polymorph.h"
#include "settings.h"
#include "dialog.h"
#include "qpc.h"
#include <tchar.h>
#include <windowsx.h>
#include <cstdint>

NOINLINE int message_loop (HWND hdlg)
{
  MSG msg;
  while (::GetMessage (& msg, NULL, 0, 0)) {
    if (! hdlg || ! ::IsDialogMessage (hdlg, & msg)) {
      ::TranslateMessage (& msg);
      ::DispatchMessage (& msg);
    }
  }
  return (int) msg.wParam;
}

int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  if (! glinit (hInstance)) return 1;

  LPCTSTR display_name;
  ::LoadString (hInstance, 1, (LPTSTR) (& display_name), 0);

  ALIGNED16 window_struct_t ws;

  get_arguments (::GetCommandLine (), ws.arguments);
  load_settings (ws.settings);

  // Create the screen saver window.
  // Retry once on failure (works around sporadic SetPixelFormat
  // failure observed on Intel integrated graphics on Windows 7).
  register_class (hInstance);
  HWND hwnd;
  for (unsigned retries = 0; retries != 2; ++ retries) {
    HWND parent = ws.arguments.mode == parented ? (HWND) ws.arguments.numeric_arg : NULL;
    hwnd = create_window (hInstance, parent, display_name, & ws);
    if (hwnd) break;
    // Window creation failed (window will be destroyed).
    // Pump messages and throw away the WM_QUIT.
    message_loop (0);
  }

  if (! hwnd) return 1;
  if (! ws.model.initialize (qpc ())) return 1;

  // Create the configure dialog if in configure mode.
  dialog_struct_t ds = { ws.settings, hwnd, };
  HWND hdlg = ws.arguments.mode == configure ? create_dialog (hInstance, & ds) : NULL;

  // Show the main window, or the configure dialog if in configure mode.
  // We use SetWindowPos because on Windows XP when the main window is
  // shown as a child of the screensaver control-panel applet windows,
  // the WM_WINDOWPOSCHANGING doesn't arrive if we use ShowWindow.
  ::SetWindowPos (hdlg ? hdlg : hwnd, NULL, 0, 0, 0, 0,
                  SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

  return message_loop (hdlg);
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
  VISIBLE NORETURN ALIGN_STACK void custom_startup ()
  {
    HINSTANCE hInstance = (HINSTANCE) & __ImageBase;
    int status = _tWinMain (hInstance, NULL, NULL, 0);
    ::ExitProcess ((UINT) (status));
  }
}

#endif
