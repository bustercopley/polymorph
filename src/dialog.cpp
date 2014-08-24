#include "dialog.h"
#include "resources.h"
#include "settings.h"
#include "reposition.h"
#include <shellapi.h>

HWND create_dialog (HINSTANCE hInstance, dialog_struct_t * ds)
{
  INITCOMMONCONTROLSEX icc = { sizeof (INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_LINK_CLASS | ICC_STANDARD_CLASSES };
  ::InitCommonControlsEx (& icc);

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
    HWND hwnd_message = ::GetDlgItem (hdlg, IDC_MESSAGE);
    HFONT font = ::CreateFont (16, 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_ROMAN, TEXT ("Segoe UI"));
    ::SendMessage (hwnd_message, WM_SETFONT, (WPARAM) font, 0);
    // The id of trackbar n is IDC_TRACKBARS_START + 4 * n, and its buddies have the two succeeding ids.
    for (unsigned i = 0; i != trackbar_count; ++ i) {
      HWND hwnd_trackbar = ::GetDlgItem (hdlg, IDC_TRACKBARS_START + 4 * i);
      ::SendMessage (hwnd_trackbar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) ds->settings.trackbar_pos [i]);
      for (unsigned j = 0; j != 2; ++ j) {
        HWND hwnd_buddy = ::GetDlgItem (hdlg, IDC_TRACKBARS_START + 4 * i + 1 + j);
        ::SendMessage (hwnd_trackbar, TBM_SETBUDDY, (WPARAM) j, (LPARAM) hwnd_buddy);
      }
    }
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
    for (unsigned i = 0; i != trackbar_count; ++ i) {
      HWND hwnd_trackbar = ::GetDlgItem (hdlg, IDC_TRACKBARS_START + 4 * i);
      ds->settings.trackbar_pos [i] = (DWORD) ::SendMessage (hwnd_trackbar, TBM_GETPOS, 0, 0);
    }
    if (id == IDOK) save_settings (ds->settings);
    if (id != IDC_PREVIEW_BUTTON) ::DestroyWindow (hdlg);
    if (id == IDC_PREVIEW_BUTTON) ::ShowWindow (ds->hwnd, SW_SHOW);
    return TRUE;
  }

  case WM_NOTIFY: {
    LPNMHDR notify = (LPNMHDR) lParam;
    if (notify->idFrom == IDC_SYSLINK && (notify->code == NM_CLICK || notify->code == NM_RETURN)) {
      PNMLINK link_notify = (PNMLINK) lParam;
      ::ShellExecute (NULL, TEXT ("open"), link_notify->item.szUrl, NULL, NULL, SW_SHOW);
    }
    return FALSE;
  }

  case WM_DRAWITEM: {
    // Draw trackbar buddy labels with baseline aligned to the bottom of the channel.
    DRAWITEMSTRUCT * drawitem = (DRAWITEMSTRUCT *) lParam;
    const UINT buddy_id = drawitem->CtlID;
    const UINT trackbar_id = buddy_id & ~3;
    HWND hwnd_buddy = drawitem->hwndItem;
    HWND hwnd_trackbar = ::GetDlgItem (hdlg, trackbar_id);
    BOOL buddy_side = (buddy_id & 3) - 1;
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
    UINT length = (UINT) ::SendMessage (hwnd_buddy, WM_GETTEXT, buffer_size, (LPARAM) buffer);
    // Render the text.
    ::SetTextAlign (drawitem->hDC, (buddy_side ? TA_RIGHT : TA_LEFT) | TA_BASELINE | TA_NOUPDATECP);
    ::ExtTextOut (drawitem->hDC, x, y, 0, NULL, buffer, length, NULL);
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
