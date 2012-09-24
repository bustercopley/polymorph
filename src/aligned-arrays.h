// -*- C++ -*-
#ifndef aligned_arrays_h
#define aligned_arrays_h

// Interface.

// p is a list of N addresses of pointers-to-elements.
// Allocate memory for a list of N arrays, each of new_length elements.
// Copy and deallocate the previous arrays if old_length is nonzero.
// Store the N pointers-to-first-element in the addresses p.
// The start of each array is aligned on a 32-byte boundary.
// The end of each array is padded with unused space to a 32-byte boundary.
// Sets memory and length to the new values.
// No-op if new_length is not greater than length (can only grow).
// Returns true for success, false for allocation failure.
template <typename ... P>
inline bool reallocate_aligned_arrays (void * & memory, unsigned & length, unsigned new_length, P ... p);

// Implementation.

#include "memory.h"
#include <cstdint>

namespace internal
{
  inline void * align_up (void * p)
  {
    return (void *) ((((std::intptr_t) p) + 31) & -32);
  }

  inline unsigned pad (unsigned p)
  {
    return (unsigned) ((((int) p) + 31) & -32);
  }

  inline unsigned bytes_needed (unsigned)
  {
    // Base case for recursion.
    // Extra bytes to align the start of the first array.
    return 32;
  }

  template <typename T, typename ... P>
  inline unsigned bytes_needed (unsigned count, T **, P ... p)
  {
    return pad (count * sizeof (T)) + bytes_needed (count, p ...);
  }

  inline void replace (void *, unsigned, unsigned)
  {
    // Base case for recursion.
  }

  template <typename T, typename ... P>
  inline void replace (void * base, unsigned old_length, unsigned new_length, T ** x, P ... p)
  {
    base = align_up (base);
    if (old_length && * x) {
      copy_memory (base, * x, old_length * (sizeof ** x));
    }
    * x = reinterpret_cast <T *> (base);
    base = reinterpret_cast <void *> (* x + new_length);
    replace (base, old_length, new_length, p ...);
  }
}

template <typename ... P>
inline bool reallocate_aligned_arrays (void * & memory, unsigned & length, unsigned new_length, P ... p)
{
  if (new_length > length) {
    unsigned bytes = internal::bytes_needed (new_length, p ...);
    void * new_memory = allocate_internal (bytes);
    if (! new_memory) {
      return false;
    }
    void * base = new_memory;
    internal::replace (base, length, new_length, p ...);
    deallocate (memory);
    memory = new_memory;
    length = new_length;
  }
  return true;
}

#endif
