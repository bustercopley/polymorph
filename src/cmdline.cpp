#include "cmdline.h"
#include <cstdint>

namespace
{
  std::uintptr_t from_string (const TCHAR * s)
  {
    std::uintptr_t result = 0;
    while (* s == TEXT (' ') || * s == TEXT (':')) ++ s;
    while (TEXT ('0') <= * s && * s <= TEXT ('9')) {
      result = 10 * result + (* s - TEXT ('0'));
      ++ s;
    }
    return result;
  }
}

run_mode_t parse_command_line (const TCHAR * s, HWND * parent_buf)
{
  run_mode_t mode = configure;
  // Skip the image path, which might be quoted.
  if (* s == TEXT ('"')) {
    ++ s;
    while (* s && * s != TEXT ('"')) ++ s;
    ++ s;
  }
  else {
    while (* s && * s != TEXT (' ')) ++ s;
  }
 repeat:
  if (TCHAR c = * s) {
    ++ s;
    c = c & TEXT ('_'); // Convert to upper case.
    if (c == TEXT ('S')) mode = fullscreen;
    else if (c == TEXT ('X')) mode = special;
    else if (c == TEXT ('P') || c == TEXT ('L')) mode = embedded;
    else goto repeat;
    * parent_buf = reinterpret_cast <HWND> (from_string (s));
  }
  return mode;
}
