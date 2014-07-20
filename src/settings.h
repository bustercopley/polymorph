// -*- C++ -*-

#ifndef settings_h
#define settings_h

#include "compiler.h"
#include "mswin.h"
#include <windowsx.h>

struct settings_t
{
  DWORD count;
  DWORD speed;
};

extern const settings_t default_settings;

void load_settings (settings_t & settings);
void save_settings (const settings_t & settings);

#endif
