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

  inline void insertion_sort (unsigned * index, const float (* x) [4], unsigned begin, unsigned end, unsigned dim)
  {
    for (unsigned n = begin + 1; n != end; ++ n) {
      // Now, the range [begin, n) is sorted.
      unsigned m = n;
      unsigned item = index [m];
      while (m > begin && x [item] [dim] < x [index [m - 1]] [dim]) {
        index [m] = index [m - 1];
        m = m - 1;
      }
      index [m] = item;
      // Now, the range [begin, n + 1) is sorted.
    }
    // Now, the range [begin, end) is sorted.
  }
}

// Reorder the range index [b, e) so that
//   {i} <= {m} for i in [b, m) and {m} <= {j} for j in (m, e),
// where {i} = x [index [i]] [dim].
// Undefined behaviour if count < 3.
void partition (unsigned * index, const float (* x) [4], const unsigned begin, const unsigned middle, const unsigned end, const unsigned dim)
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
  if (j - i > 7) partition (index, x, i, middle, j, dim);
  else if (j - i > 1) insertion_sort (index, x, i, j, dim);
}
