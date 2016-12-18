// -*- C++ -*-

// Copyright 2016 Richard Copley
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

#ifndef resource_h
#define resource_h

#include "mswin.h"

#define x64_tiny_APPNAME "Polymorph"
#define x86_tiny_APPNAME "Polymorph (x86)"
#define x64_base_APPNAME "Polymorph (base)"
#define x86_base_APPNAME "Polymorph (x86) (base)"
#define x64_debug_APPNAME "Polymorph (debug)"
#define x86_debug_APPNAME "Polymorph (x86) (debug)"

#define APPNAME_CONC0(a,b) a ## b
#define APPNAME_CONC(a,b) APPNAME_CONC0(a,b)
#define APPNAME APPNAME_CONC (PLATFORM_CONFIG, _APPNAME)

#define IDI_APPICON 1

#define IDR_VERTEX_SHADER 1
#define IDR_GEOMETRY_SHADER 2
#define IDR_FRAGMENT_SHADER 3

#define IDD_CONFIGURE 100
#define IDC_MESSAGE 101
#define IDC_PREVIEW_BUTTON 102
#define IDC_SYSLINK 103
#define IDC_BUTTONS_STATIC 104

// Must be a multiple of 4.
#define IDC_TRACKBARS_START 200

#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

#ifndef RC_INVOKED
template <typename Size>
inline void get_resource_data (int id, const char * & data, Size & size)
{
  HRSRC res = ::FindResource (0, MAKEINTRESOURCE (id), RT_RCDATA);
  size = ::SizeofResource (0, res);
  data = (const char *) ::LockResource (::LoadResource (0, res));
}
#endif

#endif
