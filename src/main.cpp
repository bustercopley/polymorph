#include "mswin.h"
#include "model.h"
#include "random.h"
#include "cmdline.h"
#include "graphics.h"
#include "config.h"
#include "memory.h"
#include "vector.h"

#define IDT_TIMER 1

struct window_struct_t
{
  model_t model;
  HDC hdc;
  HGLRC hglrc;
  LARGE_INTEGER last_performance_count;
  POINT initial_cursor_position;
  view_t view;
  float rate;
  int width, height;
  run_mode_t mode;
};

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  bool close_window = false;
  bool call_def_window_proc = false;
  if (msg == WM_CREATE) {
    const CREATESTRUCT * cs = reinterpret_cast <CREATESTRUCT *> (lParam);
    window_struct_t * ws = reinterpret_cast <window_struct_t *> (cs->lpCreateParams);
    ::SetWindowLongPtr (hwnd, GWLP_USERDATA, reinterpret_cast <LONG_PTR> (ws));
    ::GetCursorPos (& ws->initial_cursor_position);
    ws->width = cs->cx;
    ws->height = cs->cy;
#if 0
    float td = usr::tank_distance, tz = usr::tank_depth, th = usr::tank_height;
    ws->view = { td, tz, (th * ws->width) / ws->height, th, };
#else
    // Save a few instructions.
    typedef int v4i __attribute__ ((vector_size (16)));
    v4i wi = { cs->cy, cs->cy, cs->cx, cs->cy, };
    v4f hw = _mm_cvtepi32_ps ((__m128i) wi);
    v4f hh = _mm_movelh_ps (hw, hw);
    v4f ratio = hw / hh;
    v4f v = { usr::tank_distance, usr::tank_depth, usr::tank_height, usr::tank_height, };
    _mm_storeu_ps (& ws->view.distance, _mm_mul_ps (v, ratio));
#endif

    void * data (0);
    if (HRSRC data_found = ::FindResource (0, MAKEINTRESOURCE (256), MAKEINTRESOURCE (256)))
      if (HGLOBAL data_loaded = ::LoadResource (0, data_found))
        if (LPVOID data_locked = ::LockResource (data_loaded))
          data = data_locked;
    if (! data) return -1;

    LARGE_INTEGER performance_frequency;
    ::QueryPerformanceFrequency (& performance_frequency);
    ws->rate = usr::simulation_rate / performance_frequency.QuadPart;
    ::QueryPerformanceCounter (& ws->last_performance_count);

    ws->hdc = ::GetDC (hwnd);
    if (! ws->hdc) return -1;

    PIXELFORMATDESCRIPTOR pfd;
    ::ZeroMemory (& pfd, sizeof pfd);
    pfd.nSize = sizeof pfd;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    int npf = ::ChoosePixelFormat (ws->hdc, & pfd);
    ::SetPixelFormat (ws->hdc, npf, & pfd);
    ws->hglrc = ::wglCreateContext (ws->hdc);
    if (! ws->hglrc) return -1;
    if (! ::wglMakeCurrent (ws->hdc, ws->hglrc)) {
      ::wglDeleteContext (ws->hglrc);
      return -1;
    }

    typedef BOOL (WINAPI * fp_t) (int);
    if (fp_t wglSwapIntervalEXT = reinterpret_cast <fp_t> (::wglGetProcAddress ("wglSwapIntervalEXT"))) {
      wglSwapIntervalEXT (1);
    }

    screen (ws->width, ws->height);
    box (ws->view);
    lights (ws->view.distance, ws->view.depth, 3.0, 0.5, 1.0);
    clear ();

    ws->model.initialize (data, ::GetTickCount (), ws->view);
    ws->model.draw ();
    ::SetTimer (hwnd, IDT_TIMER, 1, nullptr);

    return 0;
  }
  else if (window_struct_t * ws = reinterpret_cast <window_struct_t *> (::GetWindowLongPtr (hwnd, GWLP_USERDATA))) {
    switch (msg) {
    case WM_PAINT:
      ::ValidateRect (hwnd, nullptr);
      break;
    case WM_TIMER:
      {
        ::SwapBuffers (ws->hdc);
        LARGE_INTEGER performance_count;
        ::QueryPerformanceCounter (& performance_count);
        float dt = ws->rate * (performance_count.QuadPart - ws->last_performance_count.QuadPart);
        ws->last_performance_count = performance_count;
        ws->model.proceed (dt < usr::max_frame_time ? dt : usr::max_frame_time);
        clear ();
        ws->model.draw ();
      }
      break;

    case WM_SETCURSOR:
      ::SetCursor (ws->mode == fullscreen ? nullptr : (::LoadCursor (nullptr, IDC_ARROW)));
      break;

    case WM_MOUSEMOVE:
      if (ws->mode == fullscreen) {
        // Compare the current mouse position with the one stored in the window long.
        POINT current { LOWORD (lParam), HIWORD (lParam) };
        ::ClientToScreen (hwnd, & current);
        SHORT dx = current.x - ws->initial_cursor_position.x;
        SHORT dy = current.y - ws->initial_cursor_position.y;
        close_window = dx > 10 || dy > 10 || dx * dx + dy * dy  > 100;
      }
      break;

    case WM_KEYDOWN: case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
      close_window = ws->mode == fullscreen || ws->mode == special;
      break;

    case WM_ACTIVATE: case WM_ACTIVATEAPP: case WM_NCACTIVATE:
      close_window = ws->mode == fullscreen && LOWORD (wParam) == WA_INACTIVE;
      call_def_window_proc = true;
      break;

    case WM_SYSCOMMAND:
      call_def_window_proc = (ws->mode != fullscreen || wParam != SC_SCREENSAVE) && wParam != SC_CLOSE;
      break;

    case WM_CLOSE:
      ::DestroyWindow (hwnd);
      break;

    case WM_DESTROY:
      ::wglMakeCurrent (nullptr, nullptr);
      ::wglDeleteContext (ws->hglrc);
      ::PostQuitMessage (0);
      break;

    default:
      call_def_window_proc = true;
      break;
    }
  }
  else call_def_window_proc = true;

  if (close_window) {
    ::KillTimer (hwnd, IDT_TIMER);
    ::PostMessage (hwnd, WM_CLOSE, 0, 0);
  }
  return call_def_window_proc ? ::DefWindowProc (hwnd, msg, wParam, lParam) : 0;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, int)
{
  // Parse the command line.

  HWND parent = 0;
  run_mode_t mode = parse_command_line (lpszCmdLine, & parent);
  if (mode == configure) {
    ::MessageBox (parent, usr::message, usr::message_window_name, MB_OK | MB_ICONASTERISK);
    return 0;
  }

  // Register the window class and create the screen saver window.

  DWORD style;
  DWORD ex_style;
  int left, top, width, height;

  if (mode == embedded) {
    RECT rect;
    ::GetClientRect (parent, & rect);
    left = 0;
    top = 0;
    width = rect.right;
    height = rect.bottom;
    style = WS_CHILD | WS_VISIBLE;
    ex_style = 0;
  }
  else {
    left = ::GetSystemMetrics (SM_XVIRTUALSCREEN);
    top = ::GetSystemMetrics (SM_YVIRTUALSCREEN);
    width = ::GetSystemMetrics (SM_CXVIRTUALSCREEN);
    height = ::GetSystemMetrics (SM_CYVIRTUALSCREEN);

    style = WS_POPUP | WS_VISIBLE;
    ex_style = ((mode == fullscreen) ? WS_EX_TOPMOST : 0);
  }

  WNDCLASS wc;                                    //   typedef struct {
  wc.style = 0       ;                            //     UINT style;
  wc.lpfnWndProc = WndProc;                       //     WNDPROC lpfnWndProc;
  wc.cbClsExtra = 0;                              //     int cbClsExtra;
  wc.cbWndExtra = 8;                              //     int cbWndExtra;
  wc.hInstance = hInstance;                       //     HINSTANCE hInstance;
  wc.hIcon = 0;                                   //     HICON hIcon;
  wc.hCursor = ::LoadCursor (0, IDC_ARROW);       //     HCURSOR hCursor;
  wc.hbrBackground = 0;                           //     HBRUSH hbrBackground;
  wc.lpszMenuName = 0;                            //     LPCTSTR lpszMenuName;
  wc.lpszClassName = usr::window_class_name;      //     LPCTSTR lpszClassName;
                                                  //   } WNDCLASS;
  if (! ::RegisterClass (& wc)) return 1;
  window_struct_t ws;
  ws.mode = mode;
                                                //   HWND CreateWindowEx
  HWND hwnd = ::CreateWindowEx (ex_style,       //     (DWORD dwExStyle,
        usr::window_class_name,                 //      LPCTSTR lpClassName,
        usr::window_name,                       //      LPCTSTR lpWindowName,
        style,                                  //      DWORD dwStyle,
        left,                                   //      int x,
        top,                                    //      int y,
        width,                                  //      int nWidth,
        height,                                 //      int nHeight,
        parent,                                 //      HWND hWndParent,
        0,                                      //      HMENU hMenu,
        hInstance,                              //      HINSTANCE hInstance,
        & ws);                                  //      LPVOID lpParam);

  if (! hwnd) return 1;
  // Enter the main loop.
  MSG msg;
  BOOL bRet;
  while ((bRet = ::GetMessage (& msg, nullptr, 0, 0)) != 0) {
    if (bRet == -1) {
      // TODO: check error code
      return ::GetLastError ();
    }
    else {
      ::TranslateMessage (& msg);
      ::DispatchMessage (& msg);
    }
  }

  return (msg.message == WM_QUIT ? msg.wParam : 1);
}
