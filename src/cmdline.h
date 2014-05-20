// -*- C++ -*-

#ifndef cmdline_h
#define cmdline_h

#include "mswin.h"

enum run_mode_t
{
  fullscreen, special, embedded
};

void parse_command_line (const TCHAR * s, run_mode_t & mode, bool & configure, HWND & parent_buf);

#endif
