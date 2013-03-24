// -*- C++ -*-

#ifndef show_system_h
#define show_system_h

#include "system.h"
#include <iosfwd>
#include <cstdint>

// 'show_system' produces a human-readable representation of the
// information in a 'system_t' object on the output stream 'stream'.

// Interface.

std::ostream &
show_system (std::ostream & stream,
             unsigned N, unsigned p, unsigned q, unsigned r,
             const float (* xyz) [4],
             const float (& abc) [8] [4],
             const std::uint8_t (* indices) [6]);

// Set things up so that the syntax 'stream << s', where s is a
// rotation system, results in the correct call to 'show_system'.

// Interface.

template <typename Stream, unsigned q, unsigned r>
Stream & operator << (Stream & stream, const system_t <q, r> & s);

// Implementation.

template <typename Stream, unsigned q, unsigned r>
inline Stream & operator << (Stream & stream, const system_t <q, r> & s)
{
  enum { p = 2 };
  return show_system (stream, s.N, p, q, r, s.xyz, s.abc, s.indices [0]);
}

#endif
