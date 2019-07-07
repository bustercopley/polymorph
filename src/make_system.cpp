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

#include "mswin.h"

#include "make_system.h"
#include "compiler.h"
#include "memory.h"
#include "rotor.h"
#include "vector.h"
#include <cstring>
#include <utility>

// Return the order of the rotation group of the pqr triangulation.
unsigned moebius_order (unsigned p, unsigned q, unsigned r)
{
  return (2 * p * q * r) / (q * r + r * p + p * q - p * q * r);
}

NOINLINE unsigned sigma_inverse (unsigned dart, unsigned order)
{
  if (dart % order < 1u) {
    dart += order;
  }
  return dart - 1u;
}

unsigned make_system (const unsigned (& p) [3],
                      const float (& triangle) [3] [4],
                      float (* nodes) [4], std::uint8_t (* indices) [6])
{
  ALIGNED16 rotor_t rotate;
  void * memory = allocate (3 * 6 * 60 * sizeof (unsigned));
  unsigned * alpha = reinterpret_cast <unsigned *> (memory);
  unsigned * sigma = alpha + 6 * 60;
  unsigned * origin = sigma + 6 * 60;
  const unsigned n = moebius_order (p [0], p [1], p [2]);
  unsigned next [3], a [3];

  unsigned dart = 0, node = 0;
  for (unsigned i = 0; i != 3; ++ i) {
    a [i] = dart + 1;
    next [i] = dart + 2 * p [i];
    store4f (nodes [node], load4f (triangle [i]));
    for (unsigned m = 0; m != n / p [i]; ++ m) {
      unsigned predecessor = dart + 2 * p [i] - 1;
      for (unsigned j = 0; j != 2 * p [i]; ++ j) {
        predecessor [sigma] = dart;
        origin [dart] = node;
        predecessor = dart ++;
      }
      ++ node;
    }
  }

  constexpr unsigned undef = -1;
  std::memset (alpha, '\xff', 6 * 60 * sizeof 0 [alpha]);
  unsigned mu [3] = { 0, 1, 2 };
  bool even = true;
  rotate.about (nodes [0], -6.28318531f / ui2f (p [0]));

  while (true) {
    a [0] [sigma] [alpha] = a [1];
    a [1] [sigma] [alpha] = a [2];
    a [2] [sigma] [alpha] = a [0];

    unsigned b [3];
    b [1] = sigma_inverse (a [2], 2 * p [2 [mu]]);
    if (a [2] [alpha] == undef && b [1] [alpha] != undef) {
      b [0] = sigma_inverse (b [1] [alpha], 2 * p [0 [mu]]);
      b [2] = a [1] [sigma];

      b [0] [sigma] [alpha] = b [1];
      b [1] [sigma] [alpha] = b [2];
      b [2] [sigma] [alpha] = b [0];
    }

    if (a [0] [sigma] [sigma] [alpha] == undef) {
      a [0] = a [0] [sigma];
      std::swap (1 [mu], 2 [mu]);
    }
    else if (a [1] [sigma] [sigma] [alpha] == undef) {
      a [0] = a [1] [sigma];
      std::swap (0 [mu], 1 [mu]);
      rotate.about (nodes [origin [a [0]]], -6.28318531f / ui2f (p [0 [mu]]));
    }
    else break;
    even = !even;

    if (a [0] [sigma] [sigma] [alpha] == undef) {
      a [1] = next [1 [mu]] + even;
      next [1 [mu]] += 2 * p [1 [mu]];
      rotate (nodes [origin [a [0] [alpha] [sigma] [alpha]]],
              nodes [origin [a [1]]]);
    }
    else {
      a [1] = a [0] [sigma] [sigma] [alpha] [sigma] [alpha] [sigma];
    }
    a [2] = sigma_inverse (a [0] [alpha], 2 * p [2 [mu]]);
  }

  for (unsigned i = 0; i != n / p [0]; ++ i) {
    unsigned d = 2 * p [0] * i;
    for (unsigned m = 0; m != p [0]; ++ m) {
      std::uint8_t (& block) [6] = indices [p [0] * i + m];
      block [0] = origin [d];
      block [1] = origin [d = d [alpha]];
      block [2] = origin [d = d [sigma] [alpha]];
      block [3] = origin [d = d [sigma] [sigma] [sigma] [alpha]];
      block [4] = origin [d = d [sigma] [alpha]];
      block [5] = origin [d = d [sigma] [sigma] [sigma] [alpha]];
      d = d [sigma] [alpha] [sigma];
    }
  }

  deallocate (memory);
  return n;
}
