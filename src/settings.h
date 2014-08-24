// -*- C++ -*-

#ifndef settings_h
#define settings_h

#include "compiler.h"
#include "mswin.h"
#include <windowsx.h>

struct settings_t
{
  DWORD trackbar_pos [3];
};

static const unsigned trackbar_count = 3;

void load_settings (settings_t & settings);
void save_settings (const settings_t & settings);

#endif
