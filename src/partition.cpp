#include "partition.h"
#include "compiler.h"
#include <utility>

// We don't use std::nth_element because the G++ implementation is a little bloated.
// Also beware http://gcc.gnu.org/bugzilla/show_bug.cgi?id=58800.

inline void insertion_sort (unsigned * const index, const float (* const x) [4], const unsigned dim, const unsigned begin, const unsigned end)
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
void partition (unsigned * const index, const float (* const x) [4], const unsigned dim, const unsigned begin, const unsigned middle, const unsigned end)
{
  if (end - begin <= 7) insertion_sort (index, x, dim, begin, end);
  else {
    auto val = [index, x, dim] (unsigned i) ALWAYS_INLINE { return x [index [i]] [dim]; };
    auto swap = [index] (unsigned i, unsigned j) ALWAYS_INLINE { std::swap (index [i], index [j]); };
    // Move the middle element to position 1.
    swap (begin + 1, begin + (end - begin) / 2);
    // Now put elements 0, 1, n-1 in order.
    if (val (begin) > val (begin + 1)) swap (begin, begin + 1);
    if (val (begin) > val (end - 1)) swap (begin, end - 1);
    if (val (begin + 1) > val (end - 1)) swap (begin + 1, end - 1);
    // The element now in position 1 (the median-of-three) is the pivot.
    float pivot = val (begin + 1);
    // Scan i forwards until val (i) >= pivot and scan j backwards until val (j) <= pivot;
    // if i <= j, exchange elements i and j and repeat.
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
