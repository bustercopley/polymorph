// -*- C++ -*-

#ifndef cmdline_h
#define cmdline_h

#include "mswin.h"

enum run_mode_t
{
  configure = 0, embedded, fullscreen, special
};

run_mode_t parse_command_line (const TCHAR * s, HWND * parent_buf);

#endif
