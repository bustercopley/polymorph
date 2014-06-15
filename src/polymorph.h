// -*- C++ -*-
#ifndef polymorph_h
#define polymorph_h

#include "mswin.h"
#include "cmdline.h"
#include "model.h"

struct window_struct_t
{
  model_t model;
  run_mode_t mode;
  POINT initial_cursor_position;
  HGLRC hglrc;
};

ATOM register_class (HINSTANCE hInstance);
HWND create_window (HINSTANCE hInstance, HWND parent, ATOM wndclass_id, LPCTSTR display_name, window_struct_t * ws);

#endif
