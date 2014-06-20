#include "configure.h"
#include "resources.h"
#include "settings.h"
#include "reposition.h"

#define DlgControl_GetFont(dlg, id) ((HFONT) ::SendDlgItemMessage (dlg, id, WM_GETFONT, 0, 0))
#define DlgControl_SetFont(dlg, id, font) ::SendDlgItemMessage (dlg, id, WM_SETFONT, (WPARAM) (font), 0)
#define DlgTrackBar_GetPos(dlg, id) ::SendDlgItemMessage (dlg, id, TBM_GETPOS, 0, 0)
#define DlgTrackBar_SetPos(dlg, id, pos, redraw) ::SendDlgItemMessage (dlg, id, TBM_SETPOS, (WPARAM) (redraw), (LPARAM) (pos))
#define DlgTrackBar_SetBuddy(dlg, id, end, buddy) ::SendDlgItemMessage (dlg, id, TBM_SETBUDDY, (WPARAM) (end), (LPARAM) (::GetDlgItem (dlg, buddy)))
#define DlgControl_SetText(dlg, id, text) ::SendDlgItemMessage (dlg, id, WM_SETTEXT, 0, (LPARAM) (text))

INT_PTR CALLBACK DialogProc (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  dialog_struct_t * ds = reinterpret_cast <dialog_struct_t *> (::GetWindowLongPtr (hdlg, DWLP_USER));

  switch (message)
  {
  case WM_INITDIALOG:
    {
      ds = (dialog_struct_t *) lParam;
      ::SetWindowLongPtr (hdlg, DWLP_USER, (LONG_PTR) ds);
      HFONT font = ::CreateFont (16, 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_ROMAN, TEXT ("Segoe UI"));
      DlgControl_SetFont (hdlg, IDC_MESSAGE, font);
      DlgTrackBar_SetPos (hdlg, IDC_COUNT_TRACKBAR, ds->settings->count, TRUE);
      DlgTrackBar_SetPos (hdlg, IDC_SPEED_TRACKBAR, ds->settings->speed, TRUE);
      DlgTrackBar_SetBuddy (hdlg, IDC_COUNT_TRACKBAR, FALSE, IDC_COUNT_STATIC);
      DlgTrackBar_SetBuddy (hdlg, IDC_SPEED_TRACKBAR, FALSE, IDC_SPEED_STATIC);
      reposition_window (hdlg);
      return (INT_PTR) TRUE;
    }

  case WM_CTLCOLORSTATIC: case WM_CTLCOLORDLG: {
    COLORREF color = ::GetDlgCtrlID ((HWND) lParam) == IDC_BUTTON_STATIC ? COLOR_BTNFACE : COLOR_WINDOW;
    ::SetBkColor ((HDC) wParam, ::GetSysColor (color));
    return (INT_PTR) ::GetSysColorBrush (color);
  }

  case WM_DESTROY:
    DlgControl_GetFont (hdlg, IDC_MESSAGE);
    ::DeleteObject (DlgControl_GetFont (hdlg, IDC_MESSAGE));
    ::PostQuitMessage (0);
    break;

  case WM_COMMAND:
    if (LOWORD (wParam) == IDC_PREVIEW_BUTTON || LOWORD (wParam) == IDOK) {
      ds->settings->count = DlgTrackBar_GetPos (hdlg, IDC_COUNT_TRACKBAR);
      ds->settings->speed = DlgTrackBar_GetPos (hdlg, IDC_SPEED_TRACKBAR);
    }
    if (LOWORD (wParam) == IDOK) save_settings (* ds->settings);
    if (LOWORD (wParam) == IDCANCEL || LOWORD (wParam) == IDOK) {
      ::DestroyWindow (hdlg);
      return (INT_PTR) TRUE;
    }
    else if (LOWORD (wParam) == IDC_PREVIEW_BUTTON) {
      ::ShowWindow (ds->hwnd, SW_SHOW);
      return (INT_PTR) TRUE;
    }
    break;

  // Allow the dialog manager to restore focus to the previous active control when the application is reactivated.
  case WM_NCACTIVATE:
    if (wParam) ::SetFocus (hdlg);
    return (INT_PTR) FALSE;
    break;
  }
  return (INT_PTR) FALSE;
}

HWND create_dialog (HINSTANCE hInstance, dialog_struct_t * ds)
{
  return ::CreateDialogParam (hInstance, MAKEINTRESOURCE (IDD_CONFIGURE), NULL, DialogProc, (LPARAM) ds);
}
