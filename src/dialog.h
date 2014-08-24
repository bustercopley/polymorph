// -*- C++ -*-

#ifndef dialog_h
#define dialog_h

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
