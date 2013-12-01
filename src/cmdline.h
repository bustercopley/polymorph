// -*- C++ -*-

#ifndef cmdline_h
#define cmdline_h

#include <cstdint>

enum run_mode_t
{
  configure = 0, embedded, fullscreen, special
};

run_mode_t parse_command_line (char const * s, void * parent_buf);

#endif
