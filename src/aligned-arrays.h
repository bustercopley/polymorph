// -*- C++ -*-

// Copyright 2012-2019 Richard Copley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef aligned_arrays_h
#define aligned_arrays_h

#include <cstddef>
#include <cstdint>

// Interface.

template <typename ... P>
inline void reallocate_aligned_arrays (
  void *& memory, std::size_t & capacity, std::size_t new_capacity, P *& ... p);

// Implementation.

#include "memory.h"
#include <cstdint>

namespace internal
{
  template <std::intptr_t N, typename T> inline constexpr T align_up (T p)
  {
    return (T) ((((std::intptr_t) p) + N - 1) & -N);
  }

  inline void setp (std::size_t, char *)
  {
    // Base case for recursion.
  }

  template <typename T, typename ... P>
  inline void setp (std::size_t capacity, char * base, T *& head, P *& ... rest)
  {
    head = reinterpret_cast <T *> (base);
    setp (capacity, base + capacity * sizeof (T), rest ...);
  }
}

template <typename ... P>
inline void reallocate_aligned_arrays (
  void *& memory, std::size_t & capacity, std::size_t new_capacity, P *& ... p)
{
  if (new_capacity > capacity) {
    deallocate (memory);
    capacity = internal::align_up <16> (new_capacity);
    std::size_t bytes_needed = capacity * (0 + ... + sizeof (P)) + 63;
    memory = allocate (bytes_needed);
    void * memp = internal::align_up <64> (memory);
    internal::setp (capacity, (char *) memp, p ...);
  }
}

#endif
