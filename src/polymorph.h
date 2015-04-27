// -*- C++ -*-

#ifndef polymorph_h
#define polymorph_h

#include "mswin.h"
#include "model.h"
#include "arguments.h"
#include "settings.h"

struct window_struct_t
{
  model_t model;
  arguments_t arguments;
  settings_t settings;
  POINT initial_cursor_position;
  HGLRC hglrc;
};

void register_class (HINSTANCE hInstance);
HWND create_window (HINSTANCE hInstance, HWND parent, LPCTSTR display_name, window_struct_t * ws);

#endif
