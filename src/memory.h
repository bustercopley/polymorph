// -*- C++ -*-

#ifndef memory_h
#define memory_h

void * allocate_internal (unsigned n);
void deallocate (void * p);

inline void copy_memory (void * dst, void * src, unsigned n)
{
  __builtin_memcpy (dst, src, n);
}

#endif
