// -*- C++ -*-

#include "system.h"
#include "rotor.h"
#include "triangle.h"

static const long double pi = 0x1.921fb54442d18469P1;

// Copy an array element-wise.
template <typename T, typename U, unsigned N>
void copy (const T (& from) [N], U * to)
{
  for (unsigned n = 0; n != N; ++ n)
    to [n] = from [n];
}

// Copy a two-dimensional array element-wise.
template <typename T, typename U, unsigned N, unsigned K>
void copy (const T (& from) [N] [K], U (* to) [K])
{
  for (unsigned n = 0; n != N; ++ n)
    for (unsigned k = 0; k != K; ++ k)
      to [n] [k] = from [n] [k];
}

template <unsigned q, unsigned r>
void make_system (system_t <q, r> & s)
{
  static const unsigned p = 2,
    N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r),
    Np = N / p, Nq = N / q, Nr = N / r;

  rotor_t X_rotate (2 * pi / p);
  rotor_t Y_rotate (2 * pi / q);

  long double x [Np] [4] = { { 0 } };  // Node co-ordinates.
  long double y [Nq] [4] = { { 0 } };
  long double z [Nr] [4] = { { 0 } };
  long double g [8] [4] = { { 0 } }; // Polyhedron vertices as linear combinations of nodes.

  triangle (pi / p, pi / q, pi / r, x [0], y [0], z [0], g, q, r);

  unsigned P [N], Q [N], R [N];        // Permutation taking black triangles around nodes.
  unsigned Px [N], Qx [N], Rx [N];     // The P-, Q- or R-node contained in each triangle.
  unsigned P0 [Np], R0 [Nr];           // One of the triangles around each node.

  for (unsigned n = 0; n != N; ++ n) {
    n [P] = N;
    n [Q] = n - n % q + (n + 1) % q;
    n [R] = N;
    Px [n] = n < q ? n : Np;
    Qx [n] = n / q;
    Rx [n] = n < q ? n : Nr;
    if (n < Np) P0 [n] = n < q ? n : N;
    if (n < Nr) R0 [n] = n < q ? n : N;
  }

  // We know the co-ordinates of the P- and R-nodes around Q-node 0.
  Y_rotate.about (y [0]);
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

    X_rotate.about (x [Px [n0]]);
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

    Y_rotate.about (y [Qx [m0]]);
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

  copy (x, s.xyz);
  copy (y, s.xyz + N / p );
  copy (z, s.xyz + N / p + N / q);
  copy (g, s.abc);

  for (unsigned n = 0; n != N; ++ n) {
    {
      unsigned i = n;
      unsigned j = i [R];
      unsigned k = j [P];

      s.indices [0] [n] [0] = Px [j];
      s.indices [0] [n] [1] = Qx [j] + N / p;
      s.indices [0] [n] [2] = Rx [i] + N / p + N / q;
      s.indices [0] [n] [3] = Px [i];
      s.indices [0] [n] [4] = Qx [k] + N / p;
      s.indices [0] [n] [5] = Rx [k] + N / p + N / q;
    }
    {
      unsigned j = n;
      unsigned k = j [P];
      unsigned l = k [R];
      s.indices [1] [n] [0] = Px [k];
      s.indices [1] [n] [1] = Rx [j] + N / p + N / q;
      s.indices [1] [n] [2] = Qx [k] + N / p;
      s.indices [1] [n] [3] = Px [l];
      s.indices [1] [n] [4] = Rx [k] + N / p + N / q;
      s.indices [1] [n] [5] = Qx [j] + N / p;
    }
  }
}
