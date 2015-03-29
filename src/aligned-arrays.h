// -*- C++ -*-

#ifndef aligned_arrays_h
#define aligned_arrays_h

#include <cstddef>

// Interface.

// p is a list of N addresses of pointers-to-elements.
// Allocate memory for a list of N arrays, each of new_length elements.
// Copy and deallocate the previous arrays if old_length is nonzero
// (note: the copying functionality is not currently used and is commented out).
// Store the N pointers-to-first-element in the addresses p.
// The start of each array is aligned on a 64-byte boundary.
// The end of each array is padded with unused space to a 64-byte boundary.
// Sets memory and length to the new values.
// No-op if new_length is not greater than length (can only grow).
// Returns true for success, false for allocation failure.
template <typename ... P>
inline bool reallocate_aligned_arrays (void * & memory, std::size_t & length, std::size_t new_length, P ... p);

// Implementation.

#include "memory.h"
#include <cstdint>

namespace internal
{
  template <typename T> inline T align_up (T p)
  {
    return (T) ((((std::intptr_t) p) + 63) & -64);
  }

  inline std::size_t bytes_needed (std::size_t)
  {
    // Base case for recursion.
    // Extra bytes to align the start of the first array.
    return 64;
  }

  template <typename T, typename ... P>
  inline std::size_t bytes_needed (std::size_t count, T **, P ... p)
  {
    return align_up (count * sizeof (T)) + bytes_needed (count, p ...);
  }

  inline void replace (void *, std::size_t, std::size_t)
  {
    // Base case for recursion.
  }

  template <typename T, typename ... P>
  inline void replace (void * base, std::size_t old_length, std::size_t new_length, T ** x, P ... p)
  {
    base = align_up (base);
    // Not currently used:
    //if (old_length && * x) {
    //  copy_memory (base, * x, old_length * (sizeof ** x));
    //}
    * x = reinterpret_cast <T *> (base);
    base = reinterpret_cast <void *> (* x + new_length);
    replace (base, old_length, new_length, p ...);
  }
}

template <typename ... P>
inline bool reallocate_aligned_arrays (void * & memory, std::size_t & length, std::size_t new_length, P ... p)
{
  if (new_length > length) {
    std::size_t bytes = internal::bytes_needed (new_length, p ...);
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
