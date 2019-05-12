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

#ifndef print_h
#define print_h

//#define ENABLE_PRINT

#if defined (ENABLE_PRINT) && ! defined (TINY)
#define PRINT_ENABLED 1
#else
#define PRINT_ENABLED 0
#endif

#if PRINT_ENABLED
#include "compiler.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <immintrin.h>

template <typename T, unsigned N>
inline void print (const char * name, T (& value) [N])
{
  std::cout << std::setw (12) << name;
  for (unsigned n = 0; n != N; ++ n) {
    std::cout << "  " << std::setw (12) << value [n];
  }
  std::cout << std::endl;
}

inline void print (const char * name)
{
  std::cout << name << std::endl;
}

inline void print (const wchar_t * name)
{
  std::wcout << name << std::endl;
}

template <typename T>
inline void print (const char * name, T value)
{
  T temp [1] = { value };
  print (name, temp);
}

template <>
inline void print (const char * name, __m128 value)
{
  ALIGNED16 float temp [4];
  _mm_store_ps (temp, value);
  print (name, temp);
}

inline void xassert (const char * name, bool condition)
{
  if (! (condition)) {
    std::cout << "assertion failed: " << name << "\n";
    std::exit (1);
  }
}

inline void pexit ()
{
  std::exit (1);
}

#define pp_stringize1(a) #a
#define passert(condition) xassert (pp_stringize1 (condition), condition)
#define pprint(term) print (pp_stringize1 (term), (term))

#else
template <typename T, unsigned N> inline void print (const char *, T (&) [N]) {}
template <typename T> inline void print (const char *, T) {}
inline void print (const char *) {}
inline void print (const wchar_t *) {}
inline void xassert (const char *, bool) {}
inline void pexit () {}
#define passert(condition)
#define pprint(term)
#endif
#endif
