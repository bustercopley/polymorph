// -*- C++ -*-

#ifndef aligned_arrays_h
#define aligned_arrays_h

#include <cstddef>

// Interface.

template <typename ... P>
inline void reallocate_aligned_arrays (void * & memory, std::size_t & capacity, std::size_t new_capacity, P ... p);

// Implementation.

#include "memory.h"
#include <cstdint>

namespace internal
{
  template <typename T> inline constexpr T align_up (T p, std::intptr_t alignment = 64)
  {
    return (T) ((((std::intptr_t) p) + alignment - 1) & -alignment);
  }

  inline constexpr std::size_t sum_sizes ()
  {
    return 0;
  }

  template <typename T, typename ... P> inline constexpr std::size_t sum_sizes (T **, P ... ps)
  {
    return sizeof (T) + sum_sizes (ps ...);
  }

  inline void setp (std::size_t, char *)
  {
    // Base case for recursion.
  }

  template <typename T, typename ... P>
  inline void setp (std::size_t capacity, char * base, T ** head, P ... rest)
  {
    * head = reinterpret_cast <T *> (base);
    setp (capacity, base + capacity * sizeof (T), rest ...);
  }
}

template <typename ... P>
inline void reallocate_aligned_arrays (void * & memory, std::size_t & capacity, std::size_t new_capacity, P ... p)
{
  if (new_capacity > capacity) {
    deallocate (memory);
    capacity = internal::align_up (new_capacity, 16);
    std::size_t bytes_needed = capacity * internal::sum_sizes (p ...) + 63;
    memory = allocate (bytes_needed);
    void * memp = internal::align_up (memory, 64);
    internal::setp (capacity, (char *) memp, p ...);
  }
}

#endif
