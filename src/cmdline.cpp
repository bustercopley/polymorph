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

arguments_t::arguments_t (const TCHAR * s)
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

  mode = configure;
  parent = NULL;

 repeat:
  if (TCHAR c = * s) {
    ++ s;
    c = c & TEXT ('_'); // Convert to upper case.
    if (c == TEXT ('S')) mode = screensaver;
    else if (c == TEXT ('X')) mode = persistent;
    else if (c == TEXT ('P') || c == TEXT ('L')) mode = parented;
    else goto repeat;
    while (* s == TEXT (' ') || * s == TEXT (':')) ++ s;
    parent = reinterpret_cast <HWND> (from_string (s));
  }
}
