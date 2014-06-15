#include "compiler.h"
#include "mswin.h"
#include "resources.h"
#include "glinit.h"
#include "model.h"
#include "cmdline.h"
#include "polymorph.h"
#include <tchar.h>
#include <windowsx.h>
#include <cstdint>

static const TCHAR * const message =
 TEXT ("And the ratios of their numbers, motions, and ")
 TEXT ("other properties, everywhere God, as far as ")
 TEXT ("necessity allowed or gave consent, has exactly ")
 TEXT ("perfected, and harmonised in due proportion.");

int WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
  LPCTSTR display_name;
  ::LoadString (hInstance, 1, (LPTSTR) (& display_name), 0);

  arguments_t arguments (::GetCommandLine ());

  if (arguments.mode == configure) {
    ::MessageBox (NULL, message, display_name, MB_OK | MB_ICONASTERISK);
    return 0;
  }

  if (! glinit (hInstance)) return 1;

  // Create the main window.
  window_struct_t ws ALIGNED16;
  ws.mode = arguments.mode;

  ATOM main_wndclass_atom = register_class (hInstance);
  HWND hwnd = create_window (hInstance, arguments.parent, main_wndclass_atom, display_name, & ws);
  if (! hwnd) return 1;

  ::ShowWindow (hwnd, SW_SHOW);

  // Enter the main loop.
  MSG msg;
  while (::GetMessage (& msg, NULL, 0, 0)) {
    ::TranslateMessage (& msg);
    ::DispatchMessage (& msg);
  }

  ::UnregisterClass (MAKEINTATOM (main_wndclass_atom), hInstance);
  return msg.wParam;
}

#ifdef TINY

// Tiny startup.

// No standard handles, window placement, environment variables,
// command-line transformation, global constructors and destructors,
// atexit functions, stack realignment, thread-local storage,
// runtime relocation fixups, 387 floating-point initialization,
// signal handlers or exceptions.

extern "C"
{
  // This symbol is defined by the linker.
  extern IMAGE_DOS_HEADER __ImageBase;

  // In the linker command line, specify this function as the entry point.
  void custom_startup () VISIBLE;
  void custom_startup ()
  {
    HINSTANCE hInstance = reinterpret_cast <HINSTANCE> (& __ImageBase);
    int status = _tWinMain (hInstance, NULL, NULL, 0);
    ::ExitProcess (static_cast <UINT> (status));
  }
}

#endif
