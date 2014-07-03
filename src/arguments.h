// -*- C++ -*-

#ifndef arguments_h
#define arguments_h

#include "mswin.h"

enum run_mode_t
{
  screensaver, persistent, parented, configure
};

struct arguments_t
{
  arguments_t (const TCHAR * s);
  run_mode_t mode;
  HWND parent;
};

#endif
