#include "systems.h"
#include "compiler.h"
#include "mswin.h"
#include "resources.h"
#include "vector.h"
#include "make_system.h"
#include "cramer.h"
#include "graphics.h"
#include "memory.h"
#include <cstdint>

struct triangle_t
{
  float xyz [3] [4]; // Co-ordinates for a single triangle XYZ of the Moebius tiling.
  float abc [8] [4]; // Coefficients A, B, C for the generator T = AX + BY + CZ.
};

// The fundamental triangle of the tiling has angles
// A = pi/p, B = pi/q, C = pi/r and sides a, b, c.

// By the spherical cosine rule,
// cosa = (cosA + cosB * cosC) / (sinB * sinC),
// cosb = (cosB + cosC * cosA) / (sinC * sinA),
// cosc = (cosC + cosA * cosB) / (sinA * sinB).

// We assume p = 2, so cosA = 0 and sinA = 1. Therefore,
// cosa = (cosB * cosC) / (sinB * sinC),
// cosb = cosB / sinC,
// cosc = cosC / sinB.

// A triple (alpha, beta, gamma) identifies a point T relative to the
// spherical triangle XYZ, by the formula
// T = alpha * X + beta * Y + gamma * Z.
// Certain points on the triangle generate uniform polyhedra:

// { 1, 0, 0, },             Point X.
// { 0, 1, 0, },             Point Y.
// { 0, 0, 1, },             Point Z.
// { 0, sinb, sinc, },       Intersection of YZ with the angular bisector of X.
// { sina, 0, sinc, },       Intersection of ZX with the angular bisector of Y.
// { sina, sinb, 0, },       Intersection of YZ with the angular bisector of Z.
// { sinA, sinB, sinC, },    Incentre (intersection of the bisectors).
// { alpha, beta, gamma, },  Snub generator.

//     X1
//    /  \
//   Z0--Y0
//   |\  /|
//   | X0 |
//   |/  \|
//   Y1  Z1

// For the snub generator, the three points (see diagram)
//   TX = alpha * X2 + beta * Y1 + gamma * Z1
//   TY = alpha * X1 + beta * Y2 + gamma * Z1
//   TZ = alpha * X1 + beta * Y1 + gamma * Z2
// are the vertices of an equilateral spherical triangle.
// Assuming p = 2 and q = 3, the ratio beta/alpha satisfies the
// quartic equation
//   c4*x^4 + c2*x^2 + c1*x + c0 = 0
// where
//   c4 = 64 * sinb * sinb
//   c2 = -96 * sinb * sinb
//   c1 = -54 * cosc * sina * sina
//   c0 = 36 * sinb * sinb - 27 * sina * sina
// and gamma/alpha = (sinC / 3) * (4 * (beta / alpha)^2 - 3),

triangle_t triangles [3] ALIGNED16 =
{
  // Tetrahedral, <2, 3, 3>.
  {
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x1.279a74P-1f, +0x1.a20bd8P-1f, +0x0.000000P+0f, 0.0f, },
      { +0x1.279a74P-1f, +0x0.000000P+0f, +0x1.a20bd8P-1f, 0.0f, },
    },
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x1.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x0.000000P+0f, +0x1.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x1.3988e2P-1f, +0x1.3988e2P-1f, 0.0f, },
      { +0x1.34bf64P-1f, +0x0.000000P+0f, +0x1.0b621eP-1f, 0.0f, },
      { +0x1.34bf64P-1f, +0x1.0b621eP-1f, +0x0.000000P+0f, 0.0f, },
      { +0x1.c9f25cP-2f, +0x1.8c97f0P-2f, +0x1.8c97f0P-2f, 0.0f, },
      { +0x1.4cb7c0P-2f, +0x1.d2393eP-2f, +0x1.d2393eP-2f, 0.0f, },
    },
  },
  // Octahedral, <2, 4, 3>.
  // Aligned so that two octahedral triangles cover one tetrahedral triangle.
  {
    { { +0x1.6a09e6P-1f, +0x1.000000P-1f, +0x1.000000P-1f, 0.0f, },
      { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x1.279a74P-1f, +0x1.a20bd8P-1f, +0x0.000000P+0f, 0.0f, },
    },
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x1.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x0.000000P+0f, +0x1.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x1.02ca46P-1f, +0x1.3cf3aeP-1f, 0.0f, },
      { +0x1.1fd4a6P-1f, +0x0.000000P+0f, +0x1.f28990P-2f, 0.0f, },
      { +0x1.43d136P-1f, +0x1.c9f25cP-2f, +0x0.000000P+0f, 0.0f, },
      { +0x1.b9d594P-2f, +0x1.386c8eP-2f, +0x1.7ea3c6P-2f, 0.0f, },
      { +0x1.31816eP-2f, +0x1.8d5502P-2f, +0x1.bdd092P-2f, 0.0f, },
    },
  },
  // Icosahedral, <2, 5, 3>.
  {
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x1.b38880P-1f, +0x1.0d2ca0P-1f, +0x0.000000P+0f, 0.0f, },
      { +0x1.de4bd6P-1f, +0x0.0000000P+0, +0x1.6d62c6P-2f, 0.0f, },
    },
    { { +0x1.000000P+0f, +0x0.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x1.000000P+0f, +0x0.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x0.000000P+0f, +0x1.000000P+0f, 0.0f, },
      { +0x0.000000P+0f, +0x1.b4242aP-2f, +0x1.414c80P-1f, 0.0f, },
      { +0x1.16fc4eP-1f, +0x0.000000P+0f, +0x1.e33798P-2f, 0.0f, },
      { +0x1.4e5014P-1f, +0x1.890220P-2f, +0x0.000000P+0f, 0.0f, },
      { +0x1.b3be36P-2f, +0x1.001f92P-2f, +0x1.795d50P-2f, 0.0f, },
      { +0x1.287f1eP-2f, +0x1.52a52eP-2f, +0x1.b882c6P-2f, 0.0f, },
    },
  }
};

// Turn a PQR triangle into a PRQ triangle.
void reflect (triangle_t & t)
{
  v4f x = load4f (t.xyz [0]);
  v4f y = load4f (t.xyz [1]);
  v4f z = load4f (t.xyz [2]);

  v4f n = normalize (cross (x, y));
  v4f d = dot (n, z) * n;

  store4f (t.xyz [1], z - (d + d));
  store4f (t.xyz [2], y);

  for (unsigned k = 0; k != 8; ++ k) {
    v4f a = load4f (t.abc [k]);
    store4f (t.abc [k], _mm_shuffle_ps (a, a, SHUFFLE (0, 2, 1, 3)));
  }
}

template <unsigned K>
ALWAYS_INLINE inline void copy (const float (* from) [4], float (& to) [K] [4])
{
  for (unsigned k = 0; k != K; ++ k) {
    store4f (to [k], load4f (from [k]));
  }
}

void initialize_systems (float (& abc) [system_count] [8] [4],
                         float (& xyz) [system_count] [3] [4],
                         float (& xyzinvt) [system_count] [3] [4],
                         unsigned (& primitive_count) [system_count],
                         unsigned (& vao_ids) [system_count])
{
  auto init = [&] (const triangle_t & t, system_select_t select, unsigned q, unsigned r) -> void
  {
    copy (t.xyz, xyz [select]);
    copy (t.abc, abc [select]);
    cramer::inverse_transpose (xyz [select], xyzinvt [select]);
    unsigned p = 2, N = 2 * p * q * r / (q * r + r * p + p * q - p * q * r);
    unsigned indices [60] [6];
    ALIGNED16 float nodes [62] [4];
    make_system (q, r, t.xyz, nodes, indices);
    vao_ids [select] = make_vao (N, nodes, indices);
    primitive_count [select] = N;
  };

  for (unsigned n = 0; n != 3; ++ n) {
    triangle_t & triangle = triangles [n];
    init (triangle, system_select_t (2 * n + 0), 3 + n, 3);
    reflect (triangle);
    init (triangle, system_select_t (2 * n + 1), 3, 3 + n);
  }
}
