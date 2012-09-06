// -*- C++ -*-

#ifndef partition_h
#define partition_h

#include "kdtree.h"

void partition (unsigned * index, const float (* x) [4], unsigned begin, unsigned middle, unsigned end, unsigned dim);

// Sort index in order of ascending x [i] [2].
void insertion_sort (unsigned * index, const float (* x) [4], unsigned begin, unsigned end);
void quicksort (unsigned * index, const float (* x) [4], unsigned count);

#endif
