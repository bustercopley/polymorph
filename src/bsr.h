// -*- C++ -*-

#ifndef bsr_h
#define bsr_h

#include "vector.h"
#include <cstddef>

inline unsigned bsr (unsigned x)
{
  unsigned long result;
  _BitScanReverse (& result, x);
  return (unsigned) result;
}

#endif
