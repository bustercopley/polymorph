// -*- C++ -*-
#ifndef bsr_h
#define bsr_h

#include <cstddef>

inline std::size_t bsr (std::size_t x)
{
  std::size_t result;

  asm ("bsr\t%[x], %[result]"
       : [result] "=r" (result)
       : [x] "mr" (x));

  return result;
}

#endif
