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

#ifndef polymorph_h
#define polymorph_h

#include "mswin.h"

#include "arguments.h"
#include "model.h"
#include "settings.h"
#include "print.h"

#define ENABLE_MONITOR_SELECT

#ifdef ENABLE_MONITOR_SELECT
#define MONITOR_SELECT_ENABLED 1
#else
#define MONITOR_SELECT_ENABLED 0
#endif

struct window_struct_t
{
  arguments_t arguments;
  settings_t settings;
  HINSTANCE hInstance;
  HICON icon;
  HICON icon_small;
  POINT initial_cursor_position;
  HDC hdc;
  HGLRC hglrc;
  HWND hdlg;
  HWND hwnd; // Only used by the dialogue procedure.
#if TIMING_ENABLED
  LARGE_INTEGER last_pc;
  float pf;
  unsigned frame_counter;
#endif
  model_t model;
};

HWND create_screensaver_window (window_struct_t & ws);

#endif
