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

#ifndef mswin_h
#define mswin_h

#ifdef UNICODE
#define _UNICODE
#endif

#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define WIN32_LEAN_AND_MEAN 1

// #define NOCOLOR           // Screen colors
// #define NOCTLMGR          // Control and Dialog routines
// #define NOGDI             // All GDI defines and routines
// #define NOICONS           // IDI_*
// #define NOMSG             // typedef MSG and associated routines
// #define NOSHOWWINDOW      // SW_*
// #define NOSYSCOMMANDS     // SC_*
// #define NOSYSMETRICS      // SM_*
// #define NOUSER            // All USER defines and routines
// #define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
// #define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
// #define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*

#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOMENUS           // MF_*
#define NOKEYSTATES       // MK_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOKERNEL          // All KERNEL defines and routines
#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

#include <sdkddkver.h>
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#endif
