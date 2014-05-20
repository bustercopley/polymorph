#include "cmdline.h"
#include <cstdint>

namespace
{
  std::uintptr_t from_string (const TCHAR * s)
  {
    std::uintptr_t result = 0;
    while (TEXT ('0') <= * s && * s <= TEXT ('9')) {
      result = 10 * result + (* s - TEXT ('0'));
      ++ s;
    }
    return result;
  }
}

void parse_command_line (const TCHAR * s, run_mode_t & mode, bool & configure, HWND & parent)
{
  // Skip the image path, which might be quoted.
  if (* s == TEXT ('"')) {
    ++ s;
    while (* s && * s != TEXT ('"')) ++ s;
    ++ s;
  }
  else {
    while (* s && * s != TEXT (' ')) ++ s;
  }

  mode = fullscreen;
  configure = true;
  parent = NULL;

 repeat:
  if (TCHAR c = * s) {
    ++ s;
    c = c & TEXT ('_'); // Convert to upper case.
    if (c == TEXT ('S'));
    else if (c == TEXT ('X')) mode = special;
    else if (c == TEXT ('P') || c == TEXT ('L')) mode = embedded;
    else goto repeat;
    while (* s == TEXT (' ') || * s == TEXT (':')) ++ s;
    parent = reinterpret_cast <HWND> (from_string (s));
    configure = false;
  }
}
