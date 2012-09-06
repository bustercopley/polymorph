// -*- C++ -*-

#ifndef memory_h
#define memory_h

void * allocate_internal (unsigned n);
void * reallocate_internal (void * p, unsigned n);
void deallocate (void * p);
void zero_memory (void * p, unsigned n);
void copy_memory (void * dst, void * src, unsigned n);

template <typename T>
inline T * allocate (unsigned n)
{
  return reinterpret_cast <T *> (allocate_internal (n * sizeof (T)));
}

template <typename T>
inline T * allocate ()
{
  return reinterpret_cast <T *> (allocate_internal (sizeof (T)));
}

template <typename T>
inline void reallocate (T * & p, unsigned n)
{
  p = reinterpret_cast <T *> (reallocate_internal (p, n * sizeof (T)));
}

#endif
