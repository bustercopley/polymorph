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

#ifndef compiler_h
#define compiler_h

#if defined (__GNUC__)
#define NOINLINE __attribute__ ((noinline))
#define ALWAYS_INLINE __attribute__ ((always_inline))
#define ALIGNED16 __attribute__ ((aligned (16)))
#define VISIBLE __attribute__ ((externally_visible))
#define ALIGN_STACK __attribute__ ((force_align_arg_pointer))
#define NORETURN __attribute__ ((noreturn))
#define RESTRICT __restrict__
#elif defined (_MSC_VER)
#define NOINLINE
#define ALWAYS_INLINE
#define ALIGNED16 __declspec (align (16))
#define VISIBLE
#define ALIGN_STACK
#define NORETURN __declspec (noreturn)
#define RESTRICT __restrict
#else
#define NOINLINE
#define ALWAYS_INLINE
#define ALIGNED16
#define VISIBLE
#define ALIGN_STACK
#define NORETURN
#define RESTRICT
#endif

#endif
