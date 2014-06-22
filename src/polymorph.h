// -*- C++ -*-
#ifndef polymorph_h
#define polymorph_h

#include "mswin.h"
#include "cmdline.h"
#include "model.h"
#include "settings.h"

struct window_struct_t
{
  model_t model;
  settings_t * settings;
  run_mode_t mode;
  POINT initial_cursor_position;
  HGLRC hglrc;
};

ATOM register_class (HINSTANCE hInstance);
HWND create_window (HINSTANCE hInstance, HWND parent, ATOM wndclass_id, LPCTSTR display_name, window_struct_t * ws);

#endif
