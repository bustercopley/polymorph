#include "configure.h"
#include "resources.h"
#include "settings.h"
#include "reposition.h"

#define DlgControl_GetFont(hdlg, id) ((HFONT) ::SendDlgItemMessage (hdlg, id, WM_GETFONT, 0, 0))
#define DlgControl_SetFont(hdlg, id, font) ::SendDlgItemMessage (hdlg, id, WM_SETFONT, (WPARAM) (font), 0)
#define DlgTrackBar_GetPos(hdlg, id) ::SendDlgItemMessage (hdlg, id, TBM_GETPOS, 0, 0)
#define DlgTrackBar_SetPos(hdlg, id, pos, redraw) ::SendDlgItemMessage (hdlg, id, TBM_SETPOS, (WPARAM) (redraw), (LPARAM) (pos))
#define DlgTrackBar_SetBuddy(hdlg, id, buddy, side) ::SendDlgItemMessage (hdlg, id, TBM_SETBUDDY, (WPARAM) (side), (LPARAM) (::GetDlgItem (hdlg, buddy)))

const UINT buddies [] [3] = {
  { IDC_MAX_COUNT_STATIC, IDC_COUNT_TRACKBAR, FALSE, },
  { IDC_MIN_COUNT_STATIC, IDC_COUNT_TRACKBAR, TRUE,  },
  { IDC_MAX_SPEED_STATIC, IDC_SPEED_TRACKBAR, FALSE, },
  { IDC_MIN_SPEED_STATIC, IDC_SPEED_TRACKBAR, TRUE,  },
};
const unsigned buddy_count = sizeof buddies / sizeof * buddies;

HWND create_dialog (HINSTANCE hInstance, dialog_struct_t * ds)
{
  return ::CreateDialogParam (hInstance, MAKEINTRESOURCE (IDD_CONFIGURE), NULL, DialogProc, (LPARAM) ds);
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
    DlgControl_SetFont (hdlg, IDC_MESSAGE, font);
    DlgTrackBar_SetPos (hdlg, IDC_COUNT_TRACKBAR, ds->settings->count, TRUE);
    DlgTrackBar_SetPos (hdlg, IDC_SPEED_TRACKBAR, ds->settings->speed, TRUE);
    for (unsigned i = 0; i != buddy_count; ++ i)
      DlgTrackBar_SetBuddy (hdlg, buddies [i] [1], buddies [i] [0], buddies [i] [2]);
    reposition_window (hdlg);
    return TRUE;
  }

  case WM_DESTROY:
    ::DeleteObject (DlgControl_GetFont (hdlg, IDC_MESSAGE));
    ::PostQuitMessage (0);
    return FALSE;

  case WM_COMMAND: {
    const UINT id = LOWORD (wParam);
    ds->settings->count = DlgTrackBar_GetPos (hdlg, IDC_COUNT_TRACKBAR);
    ds->settings->speed = DlgTrackBar_GetPos (hdlg, IDC_SPEED_TRACKBAR);
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
    COLORREF color = ::GetDlgCtrlID ((HWND) lParam) == IDC_BUTTONS_STATIC ? COLOR_BTNFACE : COLOR_WINDOW;
    ::SetBkColor ((HDC) wParam, ::GetSysColor (color));
    return (INT_PTR) ::GetSysColorBrush (color);
  }

  default:
    return FALSE;
  }
}
