#include "triangle.h"
#include <cmath>

inline long double sq (long double x) {
  return x * x;
}

inline long double newton (long double c0, long double c1, long double c2, long double c4) {
  // Solve the quartic equation
  //  c0 + c1.x + c2.x^2 + c4.x^4 = 0
  // numerically using Newton-Raphson iteration.

  long double x = 1.4;

  for (unsigned n = 0; n != 100; ++ n) {
    long double f = x * (x * (c4 * sq (x) + c2) + c1) + c0;
    long double df = x * (4 * c4 * sq (x) + 2 * c2) + c1;
    x -= f / df;
  }

  return x;
}

void triangle (long double A, long double B, long double C,
               long double (& u) [3], long double (& v) [3], long double (& w) [3],
               long double (& g) [8] [3]) {
  // Some light relief from the combinatorics: spherical trigonometry!
  // A, B, C are the angles of the spherical triangle XYZ.
  // Store rectangular co-ordinates for XYZ in u, v and w.
  // Store coefficients in g such that, for i = 0, 1, ..., 7, the vector
  //   g [i] [0] * u + g [i] [1] * v + g [i] [2] * w
  // identifies a point in XYZ that generates the vertices of a uniform
  // polyhedron when reflected in the spherical tiling.

  // See `Canonical co-ordinates' in the document "spherical.tex".

  long double sinA = std::sin (A), cosA = std::cos (A);
  long double sinB = std::sin (B), cosB = std::cos (B);
  long double sinC = std::sin (C), cosC = std::cos (C);

  // a, b, c are the arc lengths. Recall that spherical arc-length
  // is angle subtended at the centre. Use the spherical cosine rule:

  long double cosa = (cosA + cosB * cosC) / (sinB  * sinC);
  long double cosb = (cosB + cosC * cosA) / (sinC  * sinA);
  long double cosc = (cosC + cosA * cosB) / (sinA  * sinB);

  long double cosa_sq (sq (cosa));
  long double cosb_sq (sq (cosb));
  long double cosc_sq (sq (cosc));

  long double sina_sq (1.0 - cosa_sq);
  long double sinb_sq (1.0 - cosb_sq);
  long double sinc_sq (1.0 - cosc_sq);

  long double sina = std::sqrt (sina_sq);
  long double sinb = std::sqrt (sinb_sq);
  long double sinc = std::sqrt (sinc_sq);

  // This is all we need to draw the 16 non-chiral uniform polyhedra.
  // Continue the investigation to find numbers needed for the snubs.

  //     X2
  //    /  \    d = X1 X2             Diagram: d, e, f, h and k
  //   Z1--Y1   e = Y1 Y2             are the arc-lengths separating
  //   |\  /|   f = Z1 Z2             certain pairs of nodes in the
  //   | X1 |   h = X2 Y2             tiling of the sphere generated
  //   |/  \|   k = X2 Z2             by our spherical triangle.
  //   Y2--Z2

  long double cosA_sq (sq (cosA));
  long double cosB_sq (sq (cosB));
  long double cosC_sq (sq (cosC));

  long double cos2A = 2 * cosA_sq - 1.0;
  long double cos2B = 2 * cosB_sq - 1.0;
  long double cos2C = 2 * cosC_sq - 1.0;

  long double cos3B = (2 * cos2B - 1.0) * cosB;
  long double cos3C = (2 * cos2C - 1.0) * cosC;

  long double cosd = cosb_sq + sinb_sq * cos2C;
  long double cose = cosc_sq + sinc_sq * cos2A;
  long double cosf = cosa_sq + sina_sq * cos2B;
  long double cosh = cosa * cosb + sina * sinb * cos3C;
  long double cosk = cosa * cosc + sina * sinc * cos3B;

  // Set up a system of equations expressing the hypothesis that a
  // certain spherical triangle is equilateral. Referring to the
  // diagram above, the triangle in question is UVW, where U is
  // g(X2,Y1,Z1), V is g(X1,Y2,Z1) and W is g(X1,Y1,Z2), and where
  // the function g takes three vectors X, Y and Z and returns the
  // point alpha*X + beta*Y + gamma*Z, depending upon some
  // coefficients alpha, beta and gamma. Solving the equations
  // determines the ratios beta/alpha and gamma/alpha.

  // Set alpha=1. We get a quartic equation in beta, and a quadratic
  // expression for gamma in terms of beta.  In the tetrahedral and
  // octahedral cases, the quartic has 0 as a root, so we only need
  // to solve a cubic.  Furthermore, for the tetrahedron, beta and
  // gamma are equal, so we get a quadratic equation. The cubic for
  // the octahedron can be exactly solved using Maxima.

  // Here we just solve the quartic equation
  //   c0 + c1.x + c2.x^2 + c4.x^4 = 0
  // numerically using Newton-Raphson iteration.

  long double s = - (1 - cosf) / ((cosb - cosk) * (cosb - cosk));
  long double c0 = 1 - cosd + s * sq (1 - cosd);
  long double c1 = cosc - cosh;
  long double c2 = - 2 * s * (1 - cosd) * (1 - cose);
  long double c4 = s * sq (1 - cose);

  long double beta = newton (c0, c1, c2, c4);
  long double gamma = (cosd - 1 + (sq (beta) * (1 - cose))) / (cosb - cosk);

  // Rectangular cartesian co-ordinates of X, Y and Z.

  u [0] = 1.0;
  u [1] = 0.0;
  u [2] = 0.0;

  v [0] = cosc;
  v [1] = sinc;
  v [2] = 0.0;

  w [0] = cosb;
  w [1] = sinb * cosA;
  w [2] = sinb * sinA;

  // Coefficients for a polyhedron vertex in a map region.

  long double gx [8] [3] = {
    { 1.0, 0.0, 0.0, },
    { 0.0, 1.0, 0.0, },
    { 0.0, 0.0, 1.0, },
    { 0.0, sinb, sinc, },
    { sina, 0.0, sinc, },
    { sina, sinb, 0.0, },
    { sinA, sinB, sinC, },
    { 1.0, beta, gamma, },
  };

  // Scale the coefficients g to give a point on the unit sphere.

  for (unsigned n = 0; n != 8; ++ n) {
    long double t0 = gx [n] [0] * u [0] + gx [n] [1] * v [0] + gx [n] [2] * w [0];
    long double t1 = gx [n] [0] * u [1] + gx [n] [1] * v [1] + gx [n] [2] * w [1];
    long double t2 = gx [n] [0] * u [2] + gx [n] [1] * v [2] + gx [n] [2] * w [2];

    long double f = std::pow (sq (t0) + sq (t1) + sq (t2), - 0.5L);

    g [n] [0] = f * gx [n] [0];
    g [n] [1] = f * gx [n] [1];
    g [n] [2] = f * gx [n] [2];
  }
}
