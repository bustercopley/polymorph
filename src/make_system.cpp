#include "make_system.h"
#include "rotor.h"

// Did I mention that 'make_system' works by magic?

void make_system (unsigned q, unsigned r, const float (& xyz_in) [3] [4], float (* nodes) [4], unsigned (* indices) [6])
{
  // This is quite a lot to be allocating on the stack.
  std::uint8_t memory [350];

  std::uint8_t * P, * Q, * R; // Permutations taking black triangles around nodes.
  std::uint8_t * Px, * Rx;    // Map a triangle to its P- or R-node.
  std::uint8_t * P0, * R0;    // Map a P- or R-node to one of its triangles.

  std::uint8_t * memp = memory;
  P = memp; memp += 60;
  Q = memp; memp += 60;
  R = memp; memp += 60;
  Px = memp; memp += 60;
  Rx = memp; memp += 60;
  P0 = memp; memp += 30;
  R0 = memp; // memp += 20;

  const std::uint8_t undef = 0xff;
  const unsigned p = 2, N = 2 * p * q * r / (q * r + r * p + p * q - p * q * r);
  for (unsigned n = 0; n != 350; ++ n) memory [n] = undef;
  for (unsigned n = 0; n != N; ++ n) n [Q] = n - n % q + (n + 1) % q;
  for (unsigned n = 0; n != q; ++ n) {
    Px [n] = n;
    Rx [n] = n;
    P0 [n] = n;
    R0 [n] = n;
  }

  float (* x) [4] = nodes;
  float (* y) [4] = x + N / p;
  float (* z) [4] = y + N / q;

  store4f (x [0], load4f (xyz_in [0]));
  store4f (y [0], load4f (xyz_in [1]));
  store4f (z [0], load4f (xyz_in [2]));

  float two_pi = 0x1.921fb6P2;
  float A = two_pi / p;
  float B = two_pi / q;

  // We know the coordinates of the P- and R-nodes around Q-node 0.
  rotor_t Y_rotate (y [0], B);
  for (unsigned k = 1; k != q; ++ k) {
    Y_rotate (x [P0 [k - 1]], x [P0 [k]]);
    Y_rotate (z [R0 [k - 1]], z [R0 [k]]);
  }

  unsigned n0 = 0, p_node = q, r_node = q;
  for (unsigned m0 = q; m0 != N; m0 += q) {
    while (n0 [P] != undef) ++ n0;

    // Attach triangle m0 to triangle n0's dangling P-node.
    Px [m0] = Px [n0];

    // At this point we learn the coordinates of the next Q-node.
    // We can't yet say much about its surrounding P- and R-nodes,
    // because we don't know which of them should be identified
    // with nodes which are already known (from earlier Q-nodes)
    // and which are new.

    rotor_t X_rotate (x [Px [n0]], A);
    X_rotate (y [n0 / q], y [m0 / q]);

    // Work out the consequences of identifying the two P-nodes.
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
        n = n - n % q + (n + q - 1) % q;
        m [P] = n;
        Px [m] = Px [n] = Px [m] & Px [n];
      }
      if (n [P] != undef) {
        n = m0;
        m = n0;
      }
    } while (n [P] == undef);

    // Now all the nodes that our new triangles share with old triangles
    // are identified with nodes that are already labelled, so we can give
    // new labels to the remaining nodes and compute their vectors.

    rotor_t Y_rotate (y [m0 / q], B);
    for (unsigned n = m0 + 1; n != m0 + q; ++ n) {
      if (Px [n] == undef) {
        Px [n] = p_node;
        P0 [p_node] = n;
        ++ p_node;
        Y_rotate (x [Px [n - 1]], x [Px [n]]);
      }
      if (Rx [n] == undef) {
        Rx [n] = r_node;
        R0 [r_node] = n;
        ++ r_node;
        Y_rotate (z [Rx [n - 1]], z [Rx [n]]);
      }
    }
  }

  for (unsigned n = 0; n != N; ++ n) {
    unsigned i = n;
    unsigned j = i [R];
    unsigned k = j [P];
    indices [n] [0] = Px [j];
    indices [n] [1] = j / q + N / p;
    indices [n] [2] = Rx [i] + N / p + N / q;
    indices [n] [3] = Px [i];
    indices [n] [4] = k / q + N / p;
    indices [n] [5] = Rx [k] + N / p + N / q;
  }
}
