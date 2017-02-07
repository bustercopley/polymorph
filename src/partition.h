// -*- C++ -*-

// Copyright 2016 Richard Copley
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

#ifndef partition_h
#define partition_h

void insertion_sort (unsigned * const index, const float (* const x) [4], const unsigned dim, const unsigned begin, const unsigned end);
void partition (unsigned * index, const float (* x) [4], unsigned dim, unsigned begin, unsigned middle, unsigned end);
void qsort (unsigned * const index, const float (* const x) [4], const unsigned dim, const unsigned begin, const unsigned end);

#endif
