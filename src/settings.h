// -*- C++ -*-

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

#ifndef settings_h
#define settings_h

#include "mswin.h"

static const unsigned trackbar_count = 4;

struct settings_t
{
  DWORD trackbar_pos [trackbar_count];
};

void load_settings (settings_t & settings);
void save_settings (const settings_t & settings);

#endif
