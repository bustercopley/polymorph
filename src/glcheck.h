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

#ifndef glcheck_h
#define glcheck_h

//#define ENABLE_GLCHECK
#define GLCHECK_TRACE 0

#ifdef ENABLE_GLCHECK
#define GLCHECK_ENABLED 1
#else
#define GLCHECK_ENABLED 0
#endif

#ifdef ENABLE_GLDEBUG
#define GLDEBUG_ENABLED 1
#else
#define GLDEBUG_ENABLED 0
#endif

#endif
