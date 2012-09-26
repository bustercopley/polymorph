// -*- C++ -*-

#ifndef system_h
#define system_h

#include <cstdint>

// 'make_system' works by magic. It isn't wise to look directly into it.

//   P, Q, R, A, B, C contain the combinatorial map (see below).
//   u, v, w contain the rectagular co-ordinates of the nodes (see below).
//   g specify the 8 points of interest on the Moebius triangle (see below).

template <unsigned q, unsigned r>
struct system_t
{
  enum {
    p = 2,
    N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r)
  };

  float x [N / p] [3]; // Co-ordinates of the X-nodes.
  float y [N / q] [3]; // Co-ordinates of the Y-nodes.
  float z [N / r] [3]; // Co-ordinates of the Z-nodes.
  float g [8] [3]; // Coefficients of u, v, w for convex uniform polyhedra.

  uint8_t P [N]; // Permutation of cycle structure q^(N/q), taking triangles around X nodes.
  uint8_t Q [N]; // Permutation of cycle structure q^(N/q), taking triangles around Y nodes.
  uint8_t R [N]; // Permutation of cycle structure r^(N/r), taking triangles around Z nodes.
  uint8_t X [N]; // The X node in each triangle.
  uint8_t Y [N]; // The Y node in each triangle.
  uint8_t Z [N]; // The Z node in each triangle.
};

/*

Work in R3, the vector space of arrays of three reals. The 'sphere' is the
surface of the three-dimensional ball of unit radius, centre the origin. A
'great circle' is a circle of unit radius, centre the origin. Two points a, b on
the sphere are 'antipodal' if a + b = 0.  If a, b are non-antipodal, the
'minor great circular arc' between a and b is the smaller of the two arcs of
the great circle through a and b. The 'length' of a great circular arc is equal
to the angle subtended by the arc at the origin.

A 'spherical triangle' is a figure on the sphere consisting of three pairwise
non-antipodal points on the sphere and the three minor great circular arcs
between them. The area of a spherical triangle is equal to the 'spherical excess',
the excess of the sum of the three angles over two right angles.

If a triple (p, q, r) specifies a Moebius triangle ABC (with angles A=pi/p, B=pi/q,
C=pi/r) then we can tile the sphere once with 2N copies of this triangle, N black
and N white. (That's what makes it a Moebius triangle.) Each black tile is the image
of ABC under a direct isometry (a rotation). The white triangles are the images under
indirect isometries (reflections). Working out N from p, q and r is exercise 16.5 in
Groups and Geometry (Neumann, Stoy, Thompson); consider the area of each of three
lunes in which XYZ is contained, as a proportion of the area of the sphere.
Our combinatorial map is based on this tiling. Call its faces (tiles) 'regions',
its edges 'arcs' and its vertices 'nodes' to avoid confusion with the faces, edges
and vertice of the polyhedra discussed elsewhere. Each node is an image of one of
X, Y, Z under the rotations and reflections described above, and in consonance
therewith we speak of X nodes, Y nodes and Z nodes. There are:
  N/p X nodes, X [n] (0 <= X [n] < N/p for 0 <= n < N) (each contained in 2p regions),
  N/q Y nodes, Y [n] (0 <= Y [n] < N/q for 0 <= n < N), and
  N/r Z nodes, Z [n] (0 <= Z [n] < N/r for 0 <= n < N).
The coordinates of the nodes go into the arrays x, y and z.

P, Q and R represent permutations of the set of triangles in the spherical tiling:
triangle n [P] (respectively n [Q], n [R]) is the triangle that comes after
triangle n in the cyclic anticlockwise order of the triangles sharing triangle
n's X (respectively Y, Z) node. (Recall that n [P] = * (n + P) = * (P + n) = P [n].)

The g are co-ordinates, in a non-rectangular system, for a point P. The three vectors
0X, 0Y and 0Z, where XYZ is a Moebius triangle tile, form the basis for the system.
The g give a point P as a linear combination of X, Y and Z. That gives us 2N points
on the unit sphere: one in each region of the combinatorial map. (If P is on an edge
we only get N different points. If P=X we only get N/p. If P=Y we only get N/q. If
P=Z we only get N/r.)

*/

#endif
