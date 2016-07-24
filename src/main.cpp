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

#define WC_MAIN TEXT ("M")

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
  _mm_setcsr (_mm_getcsr () | (_MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON));

  if (! glinit (hInstance)) return 1;

  INITCOMMONCONTROLSEX icc = { sizeof (INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_LINK_CLASS | ICC_STANDARD_CLASSES };
  ::InitCommonControlsEx (& icc);

  LPCTSTR display_name;
  ::LoadString (hInstance, 1, (LPTSTR) (& display_name), 0);

  ALIGNED16 window_struct_t ws;

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
  DWORD style = (ws.arguments.mode == parented ? WS_CHILD : WS_POPUP);
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
    // Pump messages and throw away the WM_QUIT.
    message_loop (0);
  }

  if (! hwnd) return 1;

  // Window creation succeeded. Initialize the simulation.
  if (! ws.model.initialize (qpc ())) return 1;

  // Create the configure dialog if in configure mode.
  dialog_struct_t ds = { ws.settings, hwnd, };

  HWND hdlg = ws.arguments.mode == configure ? ::CreateDialogParam (hInstance, MAKEINTRESOURCE (IDD_CONFIGURE), ds.hwnd, DialogProc, (LPARAM) & ds) : NULL;

  // Show the main window, or the configure dialog if in configure mode.
  // We use SetWindowPos because on Windows 7 when the main window is
  // shown as a child of the screensaver control-panel applet windows,
  // no WM_WINDOWPOSCHANGING message arrives if we use ShowWindow.
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
