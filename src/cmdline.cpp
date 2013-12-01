#include "cmdline.h"

namespace
{
  unsigned from_string (const char * s)
  {
    std::uintptr_t result = 0;
    while (* s == ' ' || * s == ':') ++ s;
    while ('0' <= * s && * s <= '9') {
      ++ s;
      result = 10 * result + (* s - '0');
    }
    return result;
  }
}

run_mode_t parse_command_line (char const * s, void * parent_buf)
{
  run_mode_t mode = configure;
 repeat:
  if (char c = * s) {
    ++ s;
    c = c & '_'; // Convert to upper case.
    if (c == 'S') mode = fullscreen;
    else if (c == 'X') mode = special;
    else if (c == 'P' || c == 'L') mode = embedded;
    else goto repeat;
    * (std::uintptr_t *) parent_buf = from_string (s);
  }
  return mode;
}
