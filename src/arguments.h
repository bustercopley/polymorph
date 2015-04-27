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
  run_mode_t mode;
  UINT_PTR numeric_arg;
};

void get_arguments (const TCHAR * s, arguments_t & args);

#endif
