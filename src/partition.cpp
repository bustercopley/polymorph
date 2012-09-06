#include "partition.h"
#include "vector.h"

namespace
{
  inline void swap (unsigned * index, unsigned i, unsigned j)
  {
    unsigned temp = index [j];
    index [j] = index [i];
    index [i] = temp;
  }

  inline bool less (unsigned * index, const float (* x) [4], unsigned i, unsigned j, unsigned dim)
  {
    return x [index [i]] [dim] < x [index [j]] [dim];
  }
}

// Reorder index [0, count) so that {i} <= {m} for i = 0 .. m - 1,
// and {m} <= {j} for j = m + 1 .. count - 1,
// where {i} = x [index [i]] [dim].
// Undefined behaviour if count < 3.
void partition (unsigned * index, const float (* x) [4], unsigned begin, unsigned middle, unsigned end, unsigned dim)
{
  // Move the middle element to position 1.
  swap (index, begin + 1, (begin + end) / 2);
  // Now put elements 0, 1, count-1 in order.
  if (less (index, x, begin + 1, begin, dim)) swap (index, begin, begin + 1);
  if (less (index, x, end - 1, begin, dim)) swap (index, begin, end - 1);
  if (less (index, x, end - 1, begin + 1, dim)) swap (index, begin + 1, end - 1);
  // The element in position 1 (the median-of-three) is the pivot.
  // Scan i forwards for an element with {i} >= {1}, and scan j
  // backwards for {j} <= {1} (the elements in positions 1 and count-1
  // are sentinels); if i <= j, exchange elements i and j and repeat.
  unsigned i = begin + 1, j = end - 1;
  for (;;) {
    while (less (index, x, ++ i, begin + 1, dim)) continue;
    while (less (index, x, begin + 1, -- j, dim)) continue;
    if (j < i) break;
    swap (index, i, j);
  }
  // At this point j < i,
  // and: {i} >= {1}, but {k} <= {1} for k = 2 .. i-1,
  // and: {j} <= {1}, but {k} >= {1} for k = j+1 .. count-1.
  // It follows that if j < k < i then {k} == {1}.
  swap (index, begin + 1, j);
  // Now, {j} == {j+1} == ... == {i-2} == {i-1}.
  // If j <= m < i we are done; otherwise, partition
  // either [0,j) or [i,count), the one that contains m.
  if (middle < j) i = begin;
  else if (middle >= i) j = end;
  else return;
  // Partition the chosen range [i,j) about m. Tail call.
  if (j > i + 2) partition (index, x, i, middle, j, dim);
  else if (j == i + 2 && less (index, x, i + 1, i, dim)) swap (index, i, i + 1);
}

void insertion_sort (unsigned * index, const float (* x) [4], unsigned begin, unsigned end)
{
  const unsigned dim = 2;
  for (unsigned n = begin + 1; n != end; ++ n) {
    // Now, the range [begin, n) is sorted.
    unsigned item = index [n];
    while (n > begin && x [item] [dim] < x [index [n - 1]] [dim]) {
      index [n] = index [n - 1];
      n = n - 1;
    }
    index [n] = item;
    // Now, the range [begin, n + 1) is sorted.
  }
  // Now, the range [begin, end) is sorted.
}

void quicksort (unsigned * index, const float (* x) [4], unsigned count)
{
  const unsigned dim = 2;
  struct task_t {
    unsigned begin;
    unsigned end;
  };
  task_t stack [50];
  unsigned sp = 0;
  stack [sp ++] = { 0, count, };
  do {
    -- sp;
    unsigned begin = stack [sp].begin;
    unsigned end = stack [sp].end;
    if (end - begin < 7) {
      insertion_sort (index, x, begin, end);
    }
    else {
      // Move the middle element to position 1.
      swap (index, begin + 1, (begin + end) / 2);
      // Now use the median of elements 0, 1, count-1 as the pivot.
      if (less (index, x, begin + 1, begin, dim)) swap (index, begin, begin + 1);
      if (less (index, x, end - 1, begin, dim)) swap (index, begin, end - 1);
      if (less (index, x, end - 1, begin + 1, dim)) swap (index, begin + 1, end - 1);
      // Scan i forwards for an element with {i} >= {1}, and scan j
      // backwards for {j} <= {1} (the elements in positions 1 and count-1
      // are sentinels); if i < j, exchange elements i and j and repeat.
      unsigned i = begin + 1, j = end - 1;
      for (;;) {
        while (less (index, x, ++ i, begin + 1, dim)) continue;
        while (less (index, x, begin + 1, -- j, dim)) continue;
        if (j < i) break;
        swap (index, i, j);
      }
      // At this point j < i,
      // and: {i} >= {1}, but {k} <= {1} for k = 2 .. i-1,
      // and: {j} <= {1}, but {k} >= {1} for k = j+1 .. count-1.
      // It follows that if j < k < i then {k} == {1}.
      swap (index, begin + 1, j);
      // Now, {j} == {j+1} == ... == {i-2} == {i-1}.
      // Partition the ranges [begin,j) and [i,end).
      stack [sp ++] = { begin, j, };
      stack [sp ++] = { i, end, };
    }
  } while (sp);
}
