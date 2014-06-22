// -*- C++ -*-
#ifndef configure_cpp
#define configure_cpp

#include "mswin.h"
#include "settings.h"

struct dialog_struct_t
{
  settings_t * settings;
  HWND hwnd;
};

INT_PTR CALLBACK DialogProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
HWND create_dialog (HINSTANCE hInstance, dialog_struct_t * ds);

#endif
