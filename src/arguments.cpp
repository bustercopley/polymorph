// Copyright 2012-2019 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mswin.h"
#include "arguments.h"
#include "compiler.h"
#include <cstdint>

#define UPCASE(c) ((c) & ~ TEXT (' '))
#define ISUPPER(c) (TEXT ('A') <= (c) && (c) <= TEXT ('Z'))
#define ISDIGIT(c) (TEXT ('0') <= (c) && (c) <= TEXT ('9'))

ALWAYS_INLINE inline UINT_PTR number_from_string (const TCHAR * s)
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
  while (* s && ! ISDIGIT (* s) && (c = UPCASE (* s), ! ISUPPER (c))) ++ s;

  // Read optional mode letter.
  // Default to configure mode if the mode letter is missing or unrecognised.
  args.mode = configure;
  if (* s && ! ISDIGIT (* s)) {
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
