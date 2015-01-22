#include "compiler.h"
#include "mswin.h"
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "arguments.h"
#include "polymorph.h"
#include "settings.h"
#include "dialog.h"
#include <tchar.h>
#include <windowsx.h>
#include <cstdint>

int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  if (! glinit (hInstance)) return 1;

  LPCTSTR display_name;
  ::LoadString (hInstance, 1, (LPTSTR) (& display_name), 0);

  arguments_t arguments (::GetCommandLine ());

  ALIGNED16 window_struct_t ws;
  ws.hglrc = 0;
  ws.mode = arguments.mode;
  load_settings (ws.settings);

  // Create the screen saver window.
  register_class (hInstance);
  HWND hwnd = create_window (hInstance, arguments.parent, display_name, & ws);

  // Create the configure dialog if in configure mode.
  dialog_struct_t ds = { ws.settings, hwnd, };
  HWND hdlg = arguments.mode == configure ? create_dialog (hInstance, & ds) : NULL;

  // Show the main window, or the configure dialog if in configure mode.
  ::ShowWindow (hdlg ? hdlg : hwnd, SW_SHOW);

  // Enter the main loop.
  MSG msg;
  while (::GetMessage (& msg, NULL, 0, 0)) {
    if (! hdlg || ! ::IsDialogMessage (hdlg, & msg)) {
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
  VISIBLE NORETURN ALIGN_STACK void custom_startup ()
  {
    HINSTANCE hInstance = (HINSTANCE) & __ImageBase;
    int status = _tWinMain (hInstance, NULL, NULL, 0);
    ::ExitProcess ((UINT) (status));
  }
}

#endif
