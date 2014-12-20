#include "arguments.h"
#include <cstdint>

HWND hwnd_from_string (const TCHAR * s)
{
  UINT_PTR result = 0;
  while (TEXT ('0') <= * s && * s <= TEXT ('9')) {
    result = 10 * result + (* s - TEXT ('0'));
    ++ s;
  }
  return reinterpret_cast <HWND> (result);
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
    parent = hwnd_from_string (s);
  }
}
