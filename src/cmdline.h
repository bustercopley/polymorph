// -*- C++ -*-

#ifndef cmdline_h
#define cmdline_h

enum run_mode_t {
  none = 0, configure, fullscreen, embedded, special
};

run_mode_t parse_command_line (char const * s, void * parent_buf);

#endif
