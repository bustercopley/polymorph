// -*- C++ -*-

// Copyright 2012-2017 Richard Copley
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

#ifndef arguments_h
#define arguments_h

#include "mswin.h"

enum run_mode_t
{
  persistent, screensaver, configure, parented
};

struct arguments_t
{
  run_mode_t mode;
  UINT_PTR numeric_arg;
};

void get_arguments (const TCHAR * s, arguments_t & args);

#endif
