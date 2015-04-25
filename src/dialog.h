// -*- C++ -*-

#ifndef dialog_h
#define dialog_h

//#define ENABLE_REPOSITION_DIALOG
#define ENABLE_ITALICIZE_MESSAGE_FONT
#define ENABLE_OWNER_DRAWN_TRACKBAR_BUDDIES

// Disable features by commenting out definitions above.

#ifdef ENABLE_REPOSITION_DIALOG
#define REPOSITION_DIALOG_ENABLED 1
#else
#define REPOSITION_DIALOG_ENABLED 0
#endif

#ifdef ENABLE_ITALICIZE_MESSAGE_FONT
#define ITALICIZE_MESSAGE_FONT_ENABLED 1
#else
#define ITALICIZE_MESSAGE_FONT_ENABLED 0
#endif

#ifdef ENABLE_OWNER_DRAWN_TRACKBAR_BUDDIES
#define OWNER_DRAWN_TRACKBAR_BUDDIES_ENABLED 1
#else
#define OWNER_DRAWN_TRACKBAR_BUDDIES_ENABLED 0
#endif

#ifndef RC_INVOKED
#include "mswin.h"
#include "settings.h"

struct dialog_struct_t
{
  settings_t & settings;
  HWND hwnd;
};

INT_PTR CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
HWND create_dialog (HINSTANCE hInstance, dialog_struct_t * ds);
#endif

#endif
