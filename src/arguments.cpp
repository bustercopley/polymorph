#include "arguments.h"
#include <cstdint>

UINT_PTR number_from_string (const TCHAR * s)
{
  UINT_PTR result = 0;
  while (TEXT ('0') <= * s && * s <= TEXT ('9')) {
    result = 10 * result + (* s - TEXT ('0'));
    ++ s;
  }
  return result;
}

void get_arguments (const TCHAR * s, arguments_t & args)
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

  args.mode = configure;
  args.numeric_arg = 0;

 repeat:
  if (TCHAR c = * s) {
    ++ s;
    c = c & TEXT ('_'); // Convert to upper case.
    if (c == TEXT ('S')) args.mode = screensaver;
    else if (c == TEXT ('X')) args.mode = persistent;
    else if (c == TEXT ('P') || c == TEXT ('L')) args.mode = parented;
    else goto repeat;
    while (* s == TEXT (' ') || * s == TEXT (':')) ++ s;
    args.numeric_arg = number_from_string (s);
  }
}
