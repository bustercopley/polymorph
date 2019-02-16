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

#ifndef systems_h
#define systems_h

enum system_select_t {
  tetrahedral, dual_tetrahedral,
  octahedral, dual_octahedral,
  icosahedral, dual_icosahedral,
  system_count
};

void initialize_systems (float (& abc) [system_count] [8] [4],
                         float (& xyz) [system_count] [3] [4],
                         float (& xyzinv) [system_count] [3] [4],
                         unsigned (& N) [system_count],
                         unsigned (& vao_ids) [system_count]);

#endif
