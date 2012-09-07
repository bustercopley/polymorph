// -*- C++ -*-

#ifndef show_system_h
#define show_system_h

#include "system.h"
#include <iosfwd>

// 'show_system' produces a human-readable representation of the
// information in a 'system_t' object on the output stream 'stream'.

// Interface.

std::ostream &
show_system (std::ostream & stream,
             unsigned N, unsigned p, unsigned q, unsigned r,
             const unsigned char * x, const unsigned char * y, const unsigned char * z,
             const unsigned char (* s) [4],
             const float (* u) [3], const float (* v) [3], const float (* w) [3],
             const float (& k) [8] [3]);

// Set things up so that the syntax 'stream << s', where s is a
// rotation system, results in the correct call to 'show_system'.

// Interface.

template <typename Stream, unsigned q, unsigned r>
Stream & operator << (Stream & stream, const system_t <q, r> & s);

// Implementation.

template <typename Stream, unsigned q, unsigned r>
inline Stream & operator << (Stream & stream, const system_t <q, r> & s) {
  enum {
    p = 2
  };
  return show_system (stream, s.N, p, q, r, s.x, s.y, s.z, s.s, s.u, s.v, s.w, s.g);
}

#endif
