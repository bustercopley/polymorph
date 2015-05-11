#include "arguments.h"
#include <cstdint>

#define UPCASE(c) ((c) & ~ TEXT (' '))
#define ISUPPER(c) (TEXT ('A') <= (c) && (c) <= TEXT ('Z'))
#define ISDIGIT(c) (TEXT ('0') <= (c) && (c) <= TEXT ('9'))

UINT_PTR number_from_string (const TCHAR * s)
{
  UINT_PTR result = 0;
  while (ISDIGIT (* s)) {
    result = 10 * result + (* s - TEXT ('0'));
    ++ s;
  }
  return result;
}

void get_arguments (const TCHAR * s, arguments_t & args)
{
  // Skip the image path, which might be quoted.
  TCHAR c;
  bool quoted = false;
  while (c = * s, quoted ^= c == TEXT ('"'), c && (quoted || c != TEXT (' '))) ++ s;

  // Skip zero or more non-alphanumeric characters.
  c = TEXT ('\0');
  while (* s && (c = UPCASE (* s), ! (ISUPPER (c) || ISDIGIT (c)))) ++ s;

  // Read optional mode letter.
  // Default to configure mode if the mode letter is missing or unrecognised.
  args.mode = configure;
  if (ISUPPER (c)) {
    ++ s;
    if (c == TEXT ('S')) args.mode = screensaver;
    else if (c == TEXT ('X')) args.mode = persistent;
    else if (c == TEXT ('P') || c == TEXT ('L')) args.mode = parented;
    // Any other letter means configure mode.
  }

  // Skip zero or more non-numeric characters.
  while (* s && ! ISDIGIT (* s)) ++ s;

  // Read optional numeric argument.
  args.numeric_arg = number_from_string (s);
}
