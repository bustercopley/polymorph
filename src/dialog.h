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

#ifndef dialog_h
#define dialog_h

//#define ENABLE_REPOSITION_DIALOG
#define ENABLE_ITALICIZE_MESSAGE_FONT
#define ENABLE_OWNER_DRAWN_TRACKBAR_BUDDIES

// Disable features by commenting out definitions above.

#ifdef ENABLE_REPOSITION_DIALOG
#define REPOSITION_DIALOG_ENABLED 1
#else
#define REPOSITION_DIALOG_ENABLED 0
#endif

#ifdef ENABLE_ITALICIZE_MESSAGE_FONT
#define ITALICIZE_MESSAGE_FONT_ENABLED 1
#else
#define ITALICIZE_MESSAGE_FONT_ENABLED 0
#endif

#ifdef ENABLE_OWNER_DRAWN_TRACKBAR_BUDDIES
#define OWNER_DRAWN_TRACKBAR_BUDDIES_ENABLED 1
#else
#define OWNER_DRAWN_TRACKBAR_BUDDIES_ENABLED 0
#endif

#ifndef RC_INVOKED
#include "mswin.h"

INT_PTR CALLBACK DialogProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#endif
