// -*- C++ -*-

#ifndef settings_h
#define settings_h

#include "compiler.h"
#include "mswin.h"

static const unsigned trackbar_count = 4;

struct settings_t
{
  DWORD trackbar_pos [trackbar_count];
};

void load_settings (settings_t & settings);
void save_settings (const settings_t & settings);

#endif
