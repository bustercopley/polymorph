// -*- C++ -*-

#ifndef memory_h
#define memory_h

#include <cstddef>

void * allocate_internal (std::size_t n);
void deallocate (void * p);

inline void copy_memory (void * dst, const void * src, std::size_t n)
{
  __builtin_memcpy (dst, src, n);
}

#endif
