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

#include "partition.h"
#include "compiler.h"
#include <utility>

// Don't use std::nth_element because the implementation is a little bloated
// (originally because of http://gcc.gnu.org/bugzilla/show_bug.cgi?id=58800).

// Undefined behaviour if count == 0.
void insertion_sort (unsigned * const index, const float (* const x) [4],
  const unsigned dim, const unsigned begin, const unsigned end)
{
  for (unsigned n = begin + 1; n != end; ++ n) {
    // Now, the range [begin, n) is sorted.
    unsigned m = n;
    unsigned item = index [m];
    while (m != begin && x [item] [dim] < x [index [m - 1]] [dim]) {
      index [m] = index [m - 1];
      -- m;
    }
    index [m] = item;
    // Now, the range [begin, n + 1) is sorted.
  }
  // Now, the range [begin, end) is sorted.
}

// Reorder the range [begin, end) of index, so that
//   val (i) <= val (middle) for i in [begin, middle) and
//   val (j) >= val (middle) for j in (middle, end),
// where val (i) = x [index [i]] [dim].
void partition (unsigned * const index, const float (*const x) [4],
  const unsigned dim, const unsigned begin, const unsigned middle,
  const unsigned end)
{
  if (end - begin <= 7) insertion_sort (index, x, dim, begin, end);
  else {
    auto val = [index, x, dim] (unsigned i) ALWAYS_INLINE {
      return x [index [i]] [dim];
    };
    auto swap = [index] (unsigned i, unsigned j) ALWAYS_INLINE {
      std::swap (index [i], index [j]);
     };
    // Move the middle element to position 1.
    swap (begin + 1, begin + (end - begin) / 2);
    // Now put elements 0, 1, n-1 in order.
    if (val (begin) > val (begin + 1)) swap (begin, begin + 1);
    if (val (begin) > val (end - 1)) swap (begin, end - 1);
    if (val (begin + 1) > val (end - 1)) swap (begin + 1, end - 1);
    // The element now in position 1 (the median-of-three) is the pivot.
    float pivot = val (begin + 1);
    // Scan i forwards until val (i) >= pivot and scan j backwards until
    // val (j) <= pivot; if i <= j, exchange elements i and j and repeat.
    unsigned i = begin + 1, j = end - 1;
    while ([&] () -> bool {
      while (val (++ i) < pivot) continue;
      while (val (-- j) > pivot) continue;
      return i <= j;
    } ()) {
      swap (i, j);
    }
    // Now j < i,
    // and: val (i) >= pivot, but val (k) <= pivot for k = 2 .. i-1;
    // and: val (j) <= pivot, but val (k) >= pivot for k = j+1 .. count-1.
    // It follows that if j < k < i then val (k) == pivot.
    swap (begin + 1, j);
    // Now val (j) == val (j + 1) == ... == val (i - 2) == val (i - 1).
    // If j <= m < i we are done; otherwise, partition
    // either [0, j) or [i, count), the one that contains m.
    if (middle < j) i = begin;
    else if (middle >= i) j = end;
    else return;
    // Partition the chosen range [i,j) about m. Tail call.
    partition (index, x, dim, i, middle, j);
  }
}

void qsort (unsigned * const index, const float (* const x) [4], unsigned dim,
  unsigned begin, unsigned end)
{
  struct task_t
  {
    unsigned begin;
    unsigned end;
  };

  task_t stack [32];
  unsigned stackp = 0;

 loop:
  if (end - begin < 7) {
    insertion_sort (index, x, dim, begin, end);
    // Take a task off the stack.
    if (stackp) {
      -- stackp;
      begin = stack [stackp].begin;
      end = stack [stackp].end;
      goto loop;
    }
  }
  else {
    unsigned middle = begin + (end - begin) / 2;
    partition (index, x, dim, begin, middle, end);
    // Push right half onto task stack.
    stack [stackp ++] = { middle, end, };
    // Sort left half now.
    end = middle;
    goto loop;
  }
}
