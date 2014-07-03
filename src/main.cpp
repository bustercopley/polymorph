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

  settings_t settings = default_settings;
  load_settings (settings);

  // Create the main window.
  window_struct_t ws ALIGNED16;
  ws.mode = arguments.mode;
  ws.settings = & settings;
  ATOM main_wndclass_atom = register_class (hInstance);
  HWND hwnd = create_window (hInstance, arguments.parent, main_wndclass_atom, display_name, & ws);

  // Create the configure dialog if in configure mode.
  dialog_struct_t ds;
  ds.settings = & settings;
  ds.hwnd = hwnd;
  HWND hdlg = arguments.mode == configure ? create_dialog (hInstance, & ds) : NULL;

  // Show the main window, or the configure dialog if in configure mode.
  ::ShowWindow (arguments.mode == configure ? hdlg : hwnd, SW_SHOW);

  // Enter the main loop.
  MSG msg;
  while (::GetMessage (& msg, NULL, 0, 0)) {
    if (! hdlg || ! ::IsDialogMessage (hdlg, & msg)) {
      ::TranslateMessage (& msg);
      ::DispatchMessage (& msg);
    }
  }

  ::UnregisterClass (MAKEINTATOM (main_wndclass_atom), hInstance);
  return msg.wParam;
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
    HINSTANCE hInstance = reinterpret_cast <HINSTANCE> (& __ImageBase);
    int status = _tWinMain (hInstance, NULL, NULL, 0);
    ::ExitProcess (static_cast <UINT> (status));
  }
}

#endif
