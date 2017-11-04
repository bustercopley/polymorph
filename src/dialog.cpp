// Copyright 2012-2017 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mswin.h"
#include "dialog.h"
#include "polymorph.h"
#include "resources.h"
#include "settings.h"
#include "reposition.h"
#include <shellapi.h>

inline void italicize_control_font (HWND hwnd)
{
  if (HFONT font = (HFONT) ::SendMessage (hwnd, WM_GETFONT, 0, 0)) {
    ALIGNED16 LOGFONT logfont;
    if (::GetObject (font, sizeof logfont, & logfont)) {
      logfont.lfItalic = 1;
      if (HFONT font = ::CreateFontIndirect (& logfont)) { // Never deleted.
        ::SendMessage (hwnd, WM_SETFONT, (WPARAM) font, 0);
      }
    }
  }
}

INT_PTR CALLBACK DialogProc (HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  window_struct_t * ws = reinterpret_cast <window_struct_t *> (::GetWindowLongPtr (hdlg, DWLP_USER));

  switch (message) {
  case WM_INITDIALOG: {
    ws = (window_struct_t *) lParam;
    ::SetWindowLongPtr (hdlg, DWLP_USER, (LONG_PTR) ws);
    ::SendMessage (hdlg, WM_SETICON, ICON_BIG, (LPARAM) ws->icon);
    ::SendMessage (hdlg, WM_SETICON, ICON_SMALL, (LPARAM) ws->icon_small);

    // Optional extras.
    if (REPOSITION_DIALOG_ENABLED) reposition_window (hdlg);
    if (ITALICIZE_MESSAGE_FONT_ENABLED) italicize_control_font (::GetDlgItem (hdlg, IDC_MESSAGE));

    // The id of trackbar n is IDC_TRACKBARS_START + 4 * n, and its buddies have the two succeeding ids.
    for (unsigned i = 0; i != trackbar_count; ++ i) {
      HWND hwnd_trackbar = ::GetDlgItem (hdlg, IDC_TRACKBARS_START + 4 * i);
      ::SendMessage (hwnd_trackbar, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) ws->settings.trackbar_pos [i]);
      for (unsigned j = 0; j != 2; ++ j) {
        HWND hwnd_buddy = ::GetDlgItem (hdlg, IDC_TRACKBARS_START + 4 * i + 1 + j);
        ::SendMessage (hwnd_trackbar, TBM_SETBUDDY, (WPARAM) j, (LPARAM) hwnd_buddy);
      }
    }
    return TRUE;
  }

  case WM_DESTROY:
    ::PostQuitMessage (0);
    return FALSE;

  case WM_COMMAND: {
    const UINT id = LOWORD (wParam);
    for (unsigned i = 0; i != trackbar_count; ++ i) {
      HWND hwnd_trackbar = ::GetDlgItem (hdlg, IDC_TRACKBARS_START + 4 * i);
      ws->settings.trackbar_pos [i] = ::SendMessage (hwnd_trackbar, TBM_GETPOS, 0, 0);
    }
    if (id == IDOK) save_settings (ws->settings);
    if (id != IDC_PREVIEW_BUTTON) ::DestroyWindow (hdlg);
    if (id == IDC_PREVIEW_BUTTON) {
      if (! ws->hwnd) {
        ws->hwnd = create_screensaver_window (* ws);
      }
      if (ws->hwnd) {
        ::PostMessage (ws->hwnd, WM_APP, 0, 0);
      }
    }
    return TRUE;
  }

  case WM_NOTIFY: {
    LPNMHDR notify = (LPNMHDR) lParam;
    if (notify->idFrom == IDC_SYSLINK && (notify->code == NM_CLICK || notify->code == NM_RETURN)) {
      PNMLINK link_notify = (PNMLINK) lParam;
      // Since szUrl is an array of WCHAR we must use the Unicode version of ShellExecute.
      ::ShellExecuteW (NULL, L"open", link_notify->item.szUrl, NULL, NULL, SW_SHOW);
    }
    return FALSE;
  }

#if OWNER_DRAWN_TRACKBAR_BUDDIES_ENABLED
  case WM_DRAWITEM: {
    // Draw trackbar buddy labels with baseline aligned to the bottom of the channel.
    DRAWITEMSTRUCT * drawitem = (DRAWITEMSTRUCT *) lParam;
    const UINT buddy_id = drawitem->CtlID;
    const UINT trackbar_id = buddy_id & ~3;
    HWND hwnd_buddy = drawitem->hwndItem;
    HWND hwnd_trackbar = ::GetDlgItem (hdlg, trackbar_id);
    UINT alignment = (buddy_id >> 1) & 1 ? TA_RIGHT : TA_LEFT;
    // Compute the reference point in the buddy's client coordinates.
    RECT buddy_rect, trackbar_rect, channel_rect;
    ::GetWindowRect (hwnd_trackbar, & trackbar_rect);
    ::GetWindowRect (hwnd_buddy, & buddy_rect);
    ::SendMessage (hwnd_trackbar, TBM_GETCHANNELRECT, 0, (LPARAM) & channel_rect);
    int x = alignment == TA_RIGHT ? buddy_rect.right - buddy_rect.left : 0;
    int y = trackbar_rect.top + channel_rect.bottom - buddy_rect.top;
    // Retrieve the buddy's text.
    const WPARAM buffer_size = 64;
    TCHAR buffer [buffer_size];
    UINT length = (UINT) ::SendMessage (hwnd_buddy, WM_GETTEXT, buffer_size, (LPARAM) buffer);
    // Render the text.
    ::SetTextAlign (drawitem->hDC, TA_BASELINE | TA_NOUPDATECP | alignment);
    ::ExtTextOut (drawitem->hDC, x, y, 0, NULL, buffer, length, NULL);
    return TRUE;
  }
#endif

  case WM_CTLCOLORSTATIC: case WM_CTLCOLORDLG: {
    // It would make more sense to use GetDlgCtrlID, but we save a few bytes by not importing that function.
    COLORREF color = (HWND) lParam == ::GetDlgItem (hdlg, IDC_BUTTONS_STATIC) ? COLOR_BTNFACE : COLOR_WINDOW;
    return (INT_PTR) ::GetSysColorBrush (color);
  }

  default:
    return FALSE;
  }
}
