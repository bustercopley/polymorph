#include "dialog.h"
#include "resources.h"
#include "settings.h"
#include "reposition.h"

const UINT buddies [] [3] = {
  { IDC_MAX_COUNT_STATIC, IDC_COUNT_TRACKBAR, FALSE, },
  { IDC_MIN_COUNT_STATIC, IDC_COUNT_TRACKBAR, TRUE,  },
  { IDC_MAX_SPEED_STATIC, IDC_SPEED_TRACKBAR, FALSE, },
  { IDC_MIN_SPEED_STATIC, IDC_SPEED_TRACKBAR, TRUE,  },
};
const unsigned buddy_count = sizeof buddies / sizeof * buddies;

HWND create_dialog (HINSTANCE hInstance, dialog_struct_t * ds)
{
  return ::CreateDialogParam (hInstance, MAKEINTRESOURCE (IDD_CONFIGURE), ds->hwnd, DialogProc, (LPARAM) ds);
}

INT_PTR CALLBACK DialogProc (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  dialog_struct_t * ds = reinterpret_cast <dialog_struct_t *> (::GetWindowLongPtr (hdlg, DWLP_USER));

  switch (message)
  {
  case WM_INITDIALOG: {
    ds = (dialog_struct_t *) lParam;
    ::SetWindowLongPtr (hdlg, DWLP_USER, (LONG_PTR) ds);
    HFONT font = ::CreateFont (16, 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_ROMAN, TEXT ("Segoe UI"));
    ::SendMessage (::GetDlgItem (hdlg, IDC_MESSAGE), WM_SETFONT, (WPARAM) font, 0);
    HWND hwnd_count_trackbar = ::GetDlgItem (hdlg, IDC_COUNT_TRACKBAR);
    HWND hwnd_speed_trackbar = ::GetDlgItem (hdlg, IDC_SPEED_TRACKBAR);
    ::SendMessage (hwnd_count_trackbar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) ds->settings->count);
    ::SendMessage (hwnd_speed_trackbar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) ds->settings->speed);
    ::SendMessage (hwnd_count_trackbar, TBM_SETBUDDY, (WPARAM) FALSE, (LPARAM) ::GetDlgItem (hdlg, IDC_MAX_COUNT_STATIC));
    ::SendMessage (hwnd_count_trackbar, TBM_SETBUDDY, (WPARAM) TRUE, (LPARAM) ::GetDlgItem (hdlg, IDC_MIN_COUNT_STATIC));
    ::SendMessage (hwnd_speed_trackbar, TBM_SETBUDDY, (WPARAM) FALSE, (LPARAM) ::GetDlgItem (hdlg, IDC_MAX_SPEED_STATIC));
    ::SendMessage (hwnd_speed_trackbar, TBM_SETBUDDY, (WPARAM) TRUE, (LPARAM) ::GetDlgItem (hdlg, IDC_MIN_SPEED_STATIC));
    reposition_window (hdlg);
    // Override the Z order specified by the ownership relationship.
    ::SetWindowPos (hdlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    return TRUE;
  }

  case WM_DESTROY:
    ::PostQuitMessage (0);
    return FALSE;

  case WM_COMMAND: {
    const UINT id = LOWORD (wParam);
    ds->settings->count = ::SendMessage (::GetDlgItem (hdlg, IDC_COUNT_TRACKBAR), TBM_GETPOS, 0, 0);
    ds->settings->speed = ::SendMessage (::GetDlgItem (hdlg, IDC_SPEED_TRACKBAR), TBM_GETPOS, 0, 0);
    if (id == IDOK) save_settings (* ds->settings);
    if (id != IDC_PREVIEW_BUTTON) ::DestroyWindow (hdlg);
    if (id == IDC_PREVIEW_BUTTON) ::ShowWindow (ds->hwnd, SW_SHOW);
    return TRUE;
  }

  case WM_DRAWITEM: {
    // Draw trackbar buddy labels with baseline aligned to the bottom of the channel.
    DRAWITEMSTRUCT * drawitem = (DRAWITEMSTRUCT *) lParam;
    const UINT id = drawitem->CtlID;
    // Search for the trackbar of which this item is a buddy control.
    unsigned i = 0;
    while (i != buddy_count && buddies [i] [0] != id) ++ i;
    if (i != buddy_count) {
      // Found it!
      HWND hwnd_trackbar = ::GetDlgItem (hdlg, buddies [i] [1]);
      HWND hwnd_buddy = drawitem->hwndItem;
      BOOL buddy_side = buddies [i] [2];
      // Compute the reference point in the buddy's client co-ordinates.
      RECT buddy_rect, trackbar_rect, channel_rect;
      ::GetWindowRect (hwnd_trackbar, & trackbar_rect);
      ::GetWindowRect (hwnd_buddy, & buddy_rect);
      ::SendMessage (hwnd_trackbar, TBM_GETCHANNELRECT, 0, (LPARAM) & channel_rect);
      int x = (buddy_side ? buddy_rect.right : buddy_rect.left) - buddy_rect.left;
      int y = trackbar_rect.top + channel_rect.bottom - buddy_rect.top;
      // Retrieve the buddy's text.
      const WPARAM buffer_size = 64;
      TCHAR buffer [buffer_size];
      int length = ::SendMessage (hwnd_buddy, WM_GETTEXT, buffer_size, (LPARAM) buffer);
      // Render the text.
      ::SetTextAlign (drawitem->hDC, (buddy_side ? TA_RIGHT : TA_LEFT) | TA_BASELINE | TA_NOUPDATECP);
      ::ExtTextOut (drawitem->hDC, x, y, 0, NULL, buffer, length, NULL);
    }
    return TRUE;
  }

  case WM_CTLCOLORSTATIC: case WM_CTLCOLORDLG: {
    COLORREF color = (HWND) lParam == ::GetDlgItem (hdlg, IDC_BUTTONS_STATIC) ? COLOR_BTNFACE : COLOR_WINDOW;
    return (INT_PTR) ::GetSysColorBrush (color);
  }

  default:
    return FALSE;
  }
}
