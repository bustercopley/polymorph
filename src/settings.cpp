#include "settings.h"
#include "resources.h"

const settings_t default_settings = { 50, 50, };

#define COUNT_VALUE TEXT ("Count")
#define SPEED_VALUE TEXT ("Speed")
#define CONFIG_REGISTRY_KEYNAME TEXT ("SOFTWARE\\Buster\\Polymorph")

inline void load (HKEY key, LPCTSTR value_name, DWORD & setting, DWORD maximum)
{
  DWORD type, value, size;
  size = sizeof (DWORD);
  LONG error = ::RegQueryValueEx (key, value_name, 0, & type, (LPBYTE) & value, & size);
  if (error == ERROR_SUCCESS && value <= maximum) setting = value;
}

void load_settings (settings_t & settings)
{
  HKEY key;
  LONG error = ::RegOpenKeyEx (HKEY_CURRENT_USER, CONFIG_REGISTRY_KEYNAME, 0, KEY_READ, & key);
  if (error == ERROR_SUCCESS) {
    load (key, COUNT_VALUE, settings.count, 100);
    load (key, SPEED_VALUE, settings.speed, 100);
    ::RegCloseKey (key);
  }
}

void save_settings (const settings_t & settings)
{
  HKEY key;
  LONG error = ::RegCreateKeyEx (HKEY_CURRENT_USER, CONFIG_REGISTRY_KEYNAME, 0, NULL, 0, KEY_WRITE, NULL, & key, NULL);
  if (error == ERROR_SUCCESS) {
    ::RegSetValueEx (key, COUNT_VALUE, 0, REG_DWORD, (const BYTE *) & settings.count, sizeof (DWORD));
    ::RegSetValueEx (key, SPEED_VALUE, 0, REG_DWORD, (const BYTE *) & settings.speed, sizeof (DWORD));
    ::RegCloseKey (key);
  }
}
