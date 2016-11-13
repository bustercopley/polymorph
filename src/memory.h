// -*- C++ -*-

#ifndef memory_h
#define memory_h

#include <cstddef>

void * allocate (std::size_t n);
void deallocate (void * p);

#endif
