#include "arguments.h"
#include <cstdint>

#define UPCASE(c) ((c) & ~ TEXT (' '))

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
  // Default to configure mode if the mode letter is
  // missing or unrecognised.
  args.mode = configure;

  // Skip the image path, which might be quoted.
  if (* s == TEXT ('"')) {
    ++ s;
    while (* s && * s != TEXT ('"')) ++ s;
  }
  while (* s && * s != TEXT (' ')) ++ s;

  // Skip zero or more spaces.
  while (* s == TEXT (' ')) ++ s;

  // Read optional mode letter.
  TCHAR c = UPCASE (* s);
  if (TEXT ('A') <= c && c <= TEXT ('Z')) {
    ++ s;
    if (c == TEXT ('S')) args.mode = screensaver;
    else if (c == TEXT ('X')) args.mode = persistent;
    else if (c == TEXT ('P') || c == TEXT ('L')) args.mode = parented;
  }

  // Skip zero or more spaces or colons.
  while (* s == TEXT (' ') || * s == TEXT (':')) ++ s;

  // Read optional numeric argument.
  args.numeric_arg = number_from_string (s);
}
