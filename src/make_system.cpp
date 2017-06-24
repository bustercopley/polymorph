// Copyright 2012-2017 Richard Copley
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
#include "rotor.h"
#include "vector.h"

// Did I mention that 'make_system' works by magic?

void make_system (unsigned q, unsigned r, const float (& xyz_in) [3] [4], float (* nodes) [4], std::uint8_t (* indices) [6])
{
  std::uint8_t * P, * Q, * R; // Permutations taking black triangles around nodes.
  std::uint8_t * Qi;          // Inverse of the permutation Q.
  std::uint8_t * Px, * Rx;    // Map a triangle to its P- or R-node.

  std::uint8_t memory [360];
  std::uint8_t * memp = memory;
  P = memp; memp += 60;
  Q = memp; memp += 60;
  R = memp; memp += 60;
  Qi = memp; memp += 60;
  Px = memp; memp += 60;
  Rx = memp; // memp += 60;

  const std::uint8_t undef = 0xff;
  const unsigned p = 2, N = 2 * p * q * r / (q * r + r * p + p * q - p * q * r);
  for (unsigned n = 0; n != sizeof memory; ++ n) memory [n] = undef;
  for (unsigned n = 0; n != N; ++ n) {
    n [Q] = n - n % q + (n + 1) % q;
    n [Q] [Qi] = n;
  }

  unsigned next_node = N / q;

  // We are given the coordinates of the P-, Q- and R-nodes in triangle 0.
  store4f (nodes [Px [0] = next_node ++], load4f (xyz_in [0]));
  store4f (nodes [0], load4f (xyz_in [1]));
  store4f (nodes [Rx [0] = next_node ++], load4f (xyz_in [2]));

  float two_pi = 0x1.921fb6P+002f;
  float A = two_pi / ui2f (p);
  float B = two_pi / ui2f (q);

  unsigned n0 = 0, m0 = 0;

  while ([& m0, Px, Rx, q, r, nodes, & next_node, N, B] () -> bool {
    // Calculate the coordinates of any remaining unknown P- and R-nodes around Q-node m0.
    ALIGNED16 rotor_t Y_rotate (nodes [m0 / q], B);
    for (unsigned n = m0 + 1; n != m0 + q; ++ n) {
      if (Px [n] == undef) {
        Px [n] = next_node;
        Y_rotate (nodes [Px [n - 1]], nodes [next_node]);
        ++ next_node;
      }
      if (Rx [n] == undef) {
        Rx [n] = next_node;
        Y_rotate (nodes [Rx [n - 1]], nodes [next_node]);
        ++ next_node;
      }
    }
    m0 += q;
    return m0 != N;
  } ()) {
    while (n0 [P] != undef) ++ n0;

    // Attach triangle m0 to triangle n0's dangling P-node.
    // At this point we learn the coordinates of the next Q-node.
    Px [m0] = Px [n0];
    ALIGNED16 rotor_t X_rotate (nodes [Px [n0]], A);
    X_rotate (nodes [n0 / q], nodes [m0 / q]);

    // Work out the consequences of attaching the new triangle.
    // Invariant: n [P] = m if and only if m [Q] [R] = n, for all m, n < N.
    unsigned n = n0, m = m0;
    do {
      n [P] = m;
      m = m [Q];
      m [R] = n;
      Rx [m] = Rx [n] = Rx [m] & Rx [n];
      unsigned d = 1;
      while (d != r && n [R] != undef) {
        n = n [R];
        ++ d;
      }
      while (d != r && m [P] != undef) {
        m = m [P] [Q];
        ++ d;
      }
      if (d == r - 1) {
        n [R] = m;
        n = n [Qi];
        m [P] = n;
        Px [m] = Px [n] = Px [m] & Px [n];
      }
      if (n [P] != undef) {
        n = m0;
        m = n0;
      }
    } while (n [P] == undef);
  }

  for (unsigned n = 0; n != N; ++ n) {
    unsigned i = n;
    unsigned j = i [R];
    unsigned k = j [P];
    indices [n] [0] = Px [j];
    indices [n] [1] = j / q;
    indices [n] [2] = Rx [i];
    indices [n] [3] = Px [i];
    indices [n] [4] = k / q;
    indices [n] [5] = Rx [k];
  }
}
