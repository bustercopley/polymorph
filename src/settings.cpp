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

#include "settings.h"
#include "compiler.h"

struct settings_definition_t
{
  const TCHAR * registry_value_name;
  DWORD default_value;
};

const settings_definition_t settings_definitions [trackbar_count] = {
  { TEXT ("count"), 50 },
  { TEXT ("heat"), 50 },
  { TEXT ("speed"), 50 },
  { TEXT ("radius"), 50 },
};

const TCHAR * key_name = TEXT ("SOFTWARE\\Buster\\Polymorph");

inline void load (HKEY key, LPCTSTR value_name, DWORD & setting, DWORD maximum)
{
  DWORD type, value, size;
  size = sizeof (DWORD);
  LONG error =
    ::RegQueryValueEx (key, value_name, 0, & type, (LPBYTE) & value, & size);
  if (error == ERROR_SUCCESS && value <= maximum) {
    setting = value;
  }
}

NOINLINE void load_settings (settings_t & settings)
{
  for (unsigned i = 0; i != trackbar_count; ++ i) {
    settings.trackbar_pos [i] = settings_definitions [i].default_value;
  }

  HKEY key;
  LONG error =
    ::RegOpenKeyEx (HKEY_CURRENT_USER, key_name, 0, KEY_QUERY_VALUE, & key);
  if (error == ERROR_SUCCESS) {
    for (unsigned i = 0; i != trackbar_count; ++ i) {
      const TCHAR * name = settings_definitions [i].registry_value_name;
      load (key, name, settings.trackbar_pos [i], 100);
    }
    ::RegCloseKey (key);
  }
}

NOINLINE void save_settings (const settings_t & settings)
{
  HKEY key;
  LONG error = ::RegCreateKeyEx (
    HKEY_CURRENT_USER, key_name, 0, NULL, 0, KEY_SET_VALUE, NULL, & key, NULL);
  if (error == ERROR_SUCCESS) {
    for (unsigned i = 0; i != trackbar_count; ++ i) {
      const TCHAR * name = settings_definitions [i].registry_value_name;
      ::RegSetValueEx (key, name, 0, REG_DWORD,
        (const BYTE *) & settings.trackbar_pos [i], sizeof (DWORD));
    }
    ::RegCloseKey (key);
  }
}
