// -*- C++ -*-

#include "system.h"
#include "rotor.h"
#include "triangle.h"

static const double pi = 0x1.921fb54442d18P1;

// Copy a two-dimensional array element-wise.
template <typename T, typename U, unsigned N, unsigned K>
void copy (const T (& from) [N] [K], U (* to) [K]) {
  for (unsigned n = 0; n != N; ++ n)
    for (unsigned k = 0; k != K; ++ k)
      to [n] [k] = from [n] [k];
}

// Put the combinatorial information into the required form.
template <unsigned N, unsigned Np>
void transfer (const unsigned (& P) [N], const unsigned (& P0) [Np],
               const unsigned (& Qx) [N], const unsigned (& Rx) [N],
               unsigned char (& x) [2 * N]) {
  enum { p = N / Np };
  for (unsigned k = 0; k != Np; ++ k) {
    unsigned n = P0 [k];
    for (unsigned i = 0; i != p; ++ i) {
      x [2 * (k * p + i) + 0] = Qx [n];
      x [2 * (k * p + i) + 1] = Rx [n];
      n = n [P];
    }
  }
}

template <unsigned q, unsigned r>
void make_system (system_t <q, r> & s) {
  static const unsigned p = 2,
    N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r),
    Np = N / p, Nq = N / q, Nr = N / r;

  rotor_t X_rotate (2 * pi / p);
  rotor_t Y_rotate (2 * pi / q);

  long double u [Np] [3], v [Nq] [3], w [Nr] [3]; // Node co-ordinates.
  long double g [8] [3]; // Polyhedron vertices as linear combinations of nodes.

  triangle (pi / p, pi / q, pi / r, u [0], v [0], w [0], g);

  unsigned P [N], Q [N], R [N];        // Permutation taking black triangles around nodes.
  unsigned Px [N], Qx [N], Rx [N];     // The P-, Q- or R-node contained in each triangle.
  unsigned P0 [Np], Q0 [Nq], R0 [Nr];  // One of the triangles around each node.

  for (unsigned n = 0; n != N; ++ n) {
    n [P] = N;
    n [Q] = n - n % q + (n + 1) % q;
    n [R] = N;
    Px [n] = n < q ? n : Np;
    Qx [n] = n / q;
    Rx [n] = n < q ? n : Nr;
    if (n < Np) P0 [n] = n < q ? n : N;
    if (n < Nq) Q0 [n] = n * q;
    if (n < Nr) R0 [n] = n < q ? n : N;
  }

  // We know the co-ordinates of the P- and R-nodes around Q-node 0.
  Y_rotate.about (v [0]);
  for (unsigned k = 1; k != q; ++ k) {
    Y_rotate (u [P0 [k - 1]], u [P0 [k]]);
    Y_rotate (w [R0 [k - 1]], w [R0 [k]]);
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

    X_rotate.about (u [Px [n0]]);
    X_rotate (v [Qx [n0]], v [Qx [m0]]);

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

    Y_rotate.about (v [Qx [m0]]);
    for (unsigned n = m0 + 1; n != m0 + q; ++ n) {
      if (Px [n] == Np) {
        P0 [p_node] = n;
        Px [n] = p_node ++;
        Y_rotate (u [Px [n - 1]], u [Px [n]]);
      }
      if (Rx [n] == Nr) {
        R0 [r_node] = n;
        Rx [n] = r_node ++;
        Y_rotate (w [Rx [n - 1]], w [Rx [n]]);
      }
    }
  }

  transfer (P, P0, Qx, Rx, s.x);
  transfer (Q, Q0, Rx, Px, s.y);
  transfer (R, R0, Px, Qx, s.z);

  copy (u, s.u);
  copy (v, s.v);
  copy (w, s.w);
  copy (g, s.g);

  //        X0
  //       /  \      // Here we wish to associate X0, X1, X2, X3 with Xn.
  //      Z0--Y0     //
  //     /|\  /|\    //
  //   X1 | Xn | X3
  //     \|/  \|/
  //      Y1--Z1
  //       \  /
  //        X2

  for (unsigned n = 0; n != Np; ++ n) {
    unsigned a = P0 [n], b = a [P];
    s.s [n] [0] = Px [a [R]];
    s.s [n] [1] = Px [b [Q]];
    s.s [n] [2] = Px [b [R]];
    s.s [n] [3] = Px [a [Q]];
  }
}
