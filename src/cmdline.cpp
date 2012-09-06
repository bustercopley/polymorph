#include "cmdline.h"

namespace {
  unsigned from_string (const char * s) {
    unsigned value = 0;
    char c;
    while ((c = * s) && '0' <= c && c <= '9') {
      value = 10 * value + (c - '0');
      ++ s;
    }
    return value;
  }
}

run_mode_t
parse_command_line (const char * s, void * parent_buf) {
  run_mode_t mode = configure;
  unsigned * parent = reinterpret_cast <unsigned *> (parent_buf);
  * parent = 0;
 repeat:
  if (char c = * s & '_') {
    ++ s;
    if (c == 'S') mode = fullscreen;
    else if (c == 'P' || c == 'L') mode = embedded;
    else if (c == 'C') mode = configure;
    else if (c == 'X') mode = special;
    else goto repeat;
    while (* s && (* s == ' ' || * s == ':')) ++ s;
    if (mode == embedded) * parent = from_string (s);
  }
  return mode;
}
