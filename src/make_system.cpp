#include "make_system.h"
#include "vector.h"
#include "rotor.h"
#include "memory.h"

// Did I mention that 'make_system' works by magic?

void make_system (unsigned q, unsigned r, const float (& xyz_in) [3] [4], float (* xyz) [4], unsigned (* indices) [6])
{
  unsigned * P, * Q, * R;     // Permutations taking black triangles around nodes.
  unsigned * Px, * Qx, * Rx;  // Take a triangle to its P-, Q- or R-node.
  unsigned * P0, * R0;        // One of the triangles around each P- and R- node.

  // This is too much memory to allocate on the stack in one go under -nostdlib.
  void * const memory = allocate_internal (410 * sizeof (unsigned));
  unsigned * memp = (unsigned *) memory;
  P = memp; memp += 60;
  Q = memp; memp += 60;
  R = memp; memp += 60;
  Px = memp; memp += 60;
  Qx = memp; memp += 60;
  Rx = memp; memp += 60;
  P0 = memp; memp += 30;
  R0 = memp; // memp += 20;

  const unsigned p = 2, N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r), Np = N / p, Nq = N / q, Nr = N / r;

  float (* x) [4] = xyz;
  float (* y) [4] = x + Np;
  float (* z) [4] = y + Nq;

  store4f (x [0], load4f (xyz_in [0]));
  store4f (y [0], load4f (xyz_in [1]));
  store4f (z [0], load4f (xyz_in [2]));

  for (unsigned n = 0; n != N; ++ n) {
    n [P] = N;
    n [Q] = n - n % q + (n + 1) % q;
    n [R] = N;

    Qx [n] = n / q;

    if (n < q) {
      Px [n] = n;
      Rx [n] = n;
      P0 [n] = n;
      R0 [n] = n;
    }
    else
    {
      Px [n] = Np;
      Rx [n] = Nr;
      if (n < Np) P0 [n] = N;
      if (n < Nr) R0 [n] = N;
    }
  }

  float two_pi = 0x1.921fb6P2;
  float A = two_pi / p;
  float B = two_pi / q;

  // We know the co-ordinates of the P- and R-nodes around Q-node 0.
  rotor_t Y_rotate (y [0], B);
  for (unsigned k = 1; k != q; ++ k) {
    Y_rotate (x [P0 [k - 1]], x [P0 [k]]);
    Y_rotate (z [R0 [k - 1]], z [R0 [k]]);
  }

  unsigned n0 = 0, p_node = q, r_node = q;
  for (unsigned m0 = q; m0 != N; m0 += q) {
    while (n0 [P] != N) ++ n0;

    // Triangles m and n share a P-node.
    Px [m0] = Px [n0];

    // At this point we learn the co-ordinates of the next Q-node.
    // We can't yet say much about its surrounding P- and R-nodes,
    // because we don't know which of them should be identified
    // with nodes which are already known (from earlier Q-nodes)
    // and which are new.

    rotor_t X_rotate (x [Px [n0]], A);
    X_rotate (y [Qx [n0]], y [Qx [m0]]);

    // Work out the consequences of identifying the two P-nodes.
    // This is where the magic happens.

    unsigned n = n0, m = m0;
    do {
      n [P] = m;
      m = m [Q];
      m [R] = n;
      if (Rx [m] == Nr) {
        Rx [m] = Rx [n];
      }
      else if (Rx [n] == Nr) {
        Rx [n] = Rx [m];
      }
      unsigned d = 1;
      while (d != r && n [R] != N) {
        n = n [R];
        ++ d;
      }
      while (d != r && m [P] != N) {
        m = m [P] [Q];
        ++ d;
      }
      if (d == r - 1) {
        n [R] = m;
        n = n - n % q + (n + q - 1) % q;
        m [P] = n;

        if (Px [m] == Np) {
          Px [m] = Px [n];
        }
        else if (Px [n] == Np) {
          Px [n] = Px [m];
        }
      }
      if (n [P] != N) {
        n = m0;
        m = n0;
      }
    } while (n [P] == N);

    // Now all the nodes that our new triangles share with old triangles
    // are identified with nodes that are already labelled, so we can give
    // new labels to the remaining nodes and compute their vectors.

    rotor_t Y_rotate (y [Qx [m0]], B);
    for (unsigned n = m0 + 1; n != m0 + q; ++ n) {
      if (Px [n] == Np) {
        P0 [p_node] = n;
        Px [n] = p_node ++;
        Y_rotate (x [Px [n - 1]], x [Px [n]]);
      }
      if (Rx [n] == Nr) {
        R0 [r_node] = n;
        Rx [n] = r_node ++;
        Y_rotate (z [Rx [n - 1]], z [Rx [n]]);
      }
    }
  }

  for (unsigned n = 0; n != N; ++ n) {
    unsigned i = n;
    unsigned j = i [R];
    unsigned k = j [P];
    indices [n] [0] = Px [j];
    indices [n] [1] = Qx [j] + N / p;
    indices [n] [2] = Rx [i] + N / p + N / q;
    indices [n] [3] = Px [i];
    indices [n] [4] = Qx [k] + N / p;
    indices [n] [5] = Rx [k] + N / p + N / q;
  }

  deallocate (memory);
}
