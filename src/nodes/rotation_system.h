// -*- C++ -*-

#ifndef rotation_system_h
#define rotation_system_h

// 'make_system' works by magic. It isn't wise to look directly into it.

//   N, p, q, r:
//     The spherical triangle XYZ with angles A=pi/p, B=pi/q, C=pi/r
//     is the basis of an N-tile tesselation of the unit 2-sphere.
//   x, y, z each point to the start of storage for 2*N numbers.
//   s points to the start of storage for N/p arrays of 4 numbers.
//   u, v, w point to the start of storage for N/p, N/q, N/r vectors, respectively.
//   g refers to an array of 8 vectors.
//
// Post-conditions:
//   x, y, z contain the combinatorial map (see below).
//   s contains derived combinatorial data for the snubs (see below).
//   u, v, w contain the rectagular co-ordinates of the nodes (see below).
//   g specify the 8 points of interest on the Moebius triangle (see below).

template <unsigned q, unsigned r>
struct system_t {
  enum {
    p = 2,
    N = 2 * p*q*r / (q*r + r*p + p*q - p*q*r)
  };

  // There are 2N Moebius triangles in the spherical tiling (N black, N white).
  unsigned char x [2 * N]; // Indices of the Y- and Z-nodes around each X-node.
  unsigned char y [2 * N]; // Indices of the Z- and X-nodes around each Y-node.
  unsigned char z [2 * N]; // Indices of the X- and Y-nodes around each Z-node.
  unsigned char s [N / p] [4]; // Indices of the X-nodes around each X-node.
  float u [N / p] [3]; // Co-ordinates of the X-nodes.
  float v [N / q] [3]; // Co-ordinates of the Y-nodes.
  float w [N / r] [3]; // Co-ordinates of the Z-nodes.
  float g [8] [3]; // Coefficients of u, v, w for convex uniform polyhedra.
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
between them. The area of a spherical triangle is equal to the 'spherical excess'
of the sum of the three angles over two right angles.

p, q and r specify a Moebius triangle XYZ: the angles are A=pi/p, B=pi/q and C=pi/r.
We can tile the sphere once with 2N copies of this triangle, N black and N white.
(That's what makes it a Moebius triangle.) Each black triangle is the image of XYZ
under a direct isometry (a rotation). The white triangles are the images under
indirect isometries (reflections). Working out N from p, q and r is exercise 16.5
in Groups and Geometry (Neumann, Stoy, Thompson); consider the area of each of
three lunes in which XYZ is contained, as a proportion of the area of the sphere.

Our combinatorial map is based on the graph of this tiling. We'll call the
vertices nodes, the edges arcs and the faces regions, like the Americans do,
because we talk elsewhere of the vertices, edges and faces of the polyhedra we
draw. The nodes are images of X, Y and Z, and we speak of X nodes, Y nodes
and Z nodes. Similarly there are six kinds of dart, YZ, ZX, XY, ZY, XZ and YX
(a dart is half an arc) and two kinds of region, XYZ (black) and ZYX (white).

There are N/p X nodes, X_n (0 <= n < N/p) (because each is contained in 2p regions),
N/q Y nodes, Y_n (0 <= n < N/q) and N/r Z nodes, Z_n (0 <= n < N/r). Each X node
is surrounded by 2p darts, each Y node by 2q and each Z node by 2r, making for 6N
darts, that is, 3N arcs, which makes sense, because that's enough to make all N
triangular regions of one colour.

Let d_k (0 <= k < 2p) be the darts surrounding a node X_n in anticlockwise
order, so that XY darts (k even) and XZ darts (k odd) alternate. If dart d_k is
from X_n to Y_m or Z_m, then x [2np + k] = m. In other words the Y and Z nodes
neighbouring X_n are, in anticlockwise order,

  Y_(x [2np + 0]), Z_(x [2np + 1]); Y_(x [2np + 2]), Z_(x [2np + 3]); ...
  ... ; Y_(x [2np + 2p - 2]), Z_(x [2np + 2p - 1]).

// An alternative scheme is as follows. Label the darts whose first node is an X
// node d_k (0 <= k < 2N), where for 0 <= n < N/p, the darts d_k (2np <= k < 2np+2p)
// surround node X_n in that order. Do the same for the darts e_k around Y
// nodes and f_k around Z nodes. Then: if darts d_i and e_j or f_j are two darts
// of the same arc, then x [i] = j.

That explains x, y and z. For the snub polyhedra we need to know each X node's four
neighbouring X nodes in the correct order. These are easily derived from x, y and z.
They are returned in s.

Rectangular cartesian co-ordinates for all of the nodes are put into u, v and w.

The g are co-ordinates, in a non-rectangular system, for a point P. The three vectors
0X, 0Y and 0Z, where XYZ is a Moebius triangle tile, form the basis for the system.
The g give a point P as a linear combination of X, Y and Z. That gives us 2N points
on the unit sphere: one in each region of the combinatorial map. (If P is on an edge
we only get N different points. If P=X we only get N/p. If P=Y we only get N/q. If
P=Z we only get N/r.)

*/

#endif
