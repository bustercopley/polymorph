// -*- C++ -*-

#ifndef print_h
#define print_h

//#define ENABLE_PRINT

#if defined (ENABLE_PRINT) && ! defined (TINY)
#define PRINT_ENABLED 1
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <immintrin.h>
#include "compiler.h"

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
  float temp [4] ALIGNED16;
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
#define PRINT_ENABLED 0
template <typename T, unsigned N> inline void print (const char *, T (&) [N]) { }
template <typename T> inline void print (const char *, T) { }
inline void print (const char *) { }
inline void print (const wchar_t *) { }
inline void xassert (const char *, bool) { }
inline void pexit () { }
#define passert(condition)
#define pprint(term)
#endif
#endif
