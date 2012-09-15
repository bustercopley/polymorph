#include <GL/gl.h>
#include <cstdint>
#include "graphics.h"
#include "object.h"
#include "system_ref.h"
#include "frustum.h"
#include "vector.h"
#include "compiler.h"

int get_lists_start (unsigned n)
{
  return glGenLists (n);
}

void begin_list (int n)
{
  glNewList (n, GL_COMPILE);
}

void end_list ()
{
  glEndList ();
}

void lights (float distance, float depth, float lnear, float lfar, float dnear)
{
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  GLfloat lights [5] [4] = {
    { 0.00f, 0.00f, 0.00f, 1.00f, }, // Position.
    { 0.82f, 0.82f, 0.82f, 0.00f, }, // Ambient intensity.
    { 1.00f, 1.00f, 1.00f, 0.00f, }, // Diffuse intensity.
    { 1.00f, 1.00f, 1.00f, 0.00f, }, // Specular intensity.
  };

  lights [0] [2] = -distance + dnear;
  glLightfv (GL_LIGHT0, GL_POSITION, lights [0]);
  glLightfv (GL_LIGHT0, GL_AMBIENT, lights [1]);
  glLightfv (GL_LIGHT0, GL_DIFFUSE, lights [2]);
  glLightfv (GL_LIGHT0, GL_SPECULAR, lights [3]);

  v4f dd = { dnear + distance, dnear, 1.0f, 1.0f, };
  v4f ll = { lnear, lfar, lfar, lnear, };
  v4f dl = dd / ll;
  v4f hs = _mm_hsub_ps (dl, dd);
  float t [4];
  store4f (t, hs / broadcast2 (hs));

  glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION, t [0]);
  glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION, t [1]);

  GLfloat fog_color[4]= { 0.0f, 0.0f, 0.0f, 1.0f, };
  glFogi (GL_FOG_MODE, GL_LINEAR);
  glFogfv (GL_FOG_COLOR, fog_color);
  glFogf (GL_FOG_DENSITY, 0.05);
  glHint (GL_FOG_HINT, GL_NICEST);
  glFogf (GL_FOG_START, -distance);
  glFogf (GL_FOG_END, -(distance + depth));
  glEnable (GL_FOG);

  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);
  glEnable (GL_CULL_FACE);
  glEnable (GL_NORMALIZE);
  glCullFace (GL_BACK);
  glShadeModel (GL_FLAT);
  glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
  glMateriali (GL_FRONT, GL_SHININESS, 12);
  glDisable (GL_DEPTH_TEST);
}

void screen (int width, int height)
{
  glViewport (0, 0, width, height);
}

void box (const view_t & view)
{
  v4f v = _mm_loadu_ps (& view.distance); // z1 zd w h
  v4f k = { 0.5f, 0.5f, 0.5f, 0.5f, };
  v4f s0 = k * _mm_movehl_ps (v, v);      // x1 y1 x1 y1
  v4f s1 = _mm_hadd_ps (v, v);            // z2 * z2 *
  v2d xp = _mm_cvtps_pd (s0);
  v2d neg = { -0.0, -0.0, };
  v2d xm = _mm_xor_pd (xp, neg);
  v2d z = _mm_cvtps_pd (_mm_unpacklo_ps (v, s1));
  double t [6] ALIGNED16;
  _mm_store_pd (& t [0], xm);
  _mm_store_pd (& t [2], xp);
  _mm_store_pd (& t [4], z);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  // glFrustum's z arguments are specified as positive distances,
  // but the z co-ordinate of a point inside the viewing frustum
  // is negative (in eye co-ordinates).
  glFrustum (t [0], t [2], t [1], t [3], t [4], t [5]);
}

void clear ()
{
  glClear (GL_COLOR_BUFFER_BIT);
}

namespace
{
  inline void point (v4f ax, v4f by, v4f cz)
  {
    // Send a vertex `p' to the graphics library. `p' is the sum of `ax', `by' and `cz'.

    // The vector `ax' is the product of a scalar `a' and a vector 'x', and so on.
    // In other words, `a', `b', `c' are the co-ordinates of `p' in the co-ordinate
    // system defined by the basis (`x', `y', `z').

    // `x', `y' and `z' are nodes of a Moebius triangle in the spherical tiling.
    // `a', `b' and `c' have been chosen so that `p' lies on the unit sphere.
    // Certain values of the co-ordinates `a', `b', `c' pick out vertices of
    // uniform polyhedra: see "nodes/make_system.tcc" for details.
    GLfloat p [4] ALIGNED16;
    store4f (p, ax + by + cz);
    glVertex3fv (p);
  }
}

void paint_kpgons (unsigned k, unsigned Np, unsigned p,
                   const uint8_t * P,
                   const uint8_t * Y, const uint8_t * Z,
                   const float (* x) [4],
                   const float (* ax) [4], const float (* by) [4], const float (* cz) [4])
{
  if (k == 1) {
    for (unsigned n = 0; n != Np; ++ n) {
      glBegin (GL_POLYGON);
      glNormal3fv (x [n]);
      v4f x = load4f (ax [n]);
      v4f y = load4f (by [Y [P [n * p + 0]]]);
      v4f z = load4f (cz [Z [P [n * p + p - 1]]]);
      point (x, y, z);
      for (unsigned k0 = 1; k0 != p; ++ k0) {
        v4f y = load4f (by [Y [P [n * p + k0]]]);
        v4f z = load4f (cz [Z [P [n * p + k0 - 1]]]);
        point (x, y, z);
      }
      glEnd ();
    }
  }
  else if (k == 2) {
    for (unsigned n = 0; n != Np; ++ n) {
      glBegin (GL_POLYGON);
      glNormal3fv (x [n]);
      v4f x = load4f (ax [n]);
      v4f z = load4f (cz [Z [P [n * p + p - 1]]]);
      for (unsigned k0 = 0; k0 != p; ++ k0) {
        v4f y = load4f (by [Y [P [n * p + k0]]]);
        point (x, y, z);
        z = load4f (cz [Z [P [n * p + k0]]]);
        point (x, y, z);
      }
      glEnd ();
    }
  }
}

void
paint_snub_pgons (int chirality, unsigned Np, unsigned p,
                  const uint8_t * P,
                  const uint8_t * Y, const uint8_t * Z,
                  const float (* u) [4],
                  const float (* ax) [4], const float (* by) [4], const float (* cz) [4])
{
  for (unsigned n = 0; n != Np; ++ n) {
    glBegin (GL_POLYGON);
    glNormal3fv (u [n]);
    v4f x = load4f (ax [n]);
    if (chirality == 1) {
      // By considering the p black tiles around each p-node,
      // obtain N/p p-gonal faces.
      for (unsigned k = 0; k != p; ++ k)
        point (x,
               load4f (by [Y [P [n * p + k]]]),
               load4f (cz [Z [P [n * p + k]]]));
    }
    else {
      // The same, but with the white tiles.
      point (x,
             load4f (by [Y [P [n * p + 0]]]),
             load4f (cz [Z [P [n * p + p - 1]]]));
      for (unsigned k = 1; k != p; ++ k)
        point (x,
               load4f (by [Y [P [n * p + k]]]),
               load4f (cz [Z [P [n * p + k - 1]]]));
    }
    glEnd ();
  }
}

void
paint_snub_triangle_pairs (int chirality, unsigned Np,
                           const uint8_t * P, const uint8_t * s,
                           const uint8_t * Y, const uint8_t * Z,
                           const float (* ax) [4], const float (* by) [4], const float (* cz) [4])
{
  for (unsigned n = 0; n != Np; ++ n) {               //        X1        //
    v4f a, b, c, d;                                   //       /  \       //
    v4f x0 = load4f (ax [n]);                         //      Z0--Y0      //
    v4f y0 = load4f (by [Y [P [2 * n + 0]]]);         //     /|\  /|\     //
    v4f z0 = load4f (cz [Z [P [2 * n + 0]]]);         //   X4 | X0 | X2   //
    v4f y1 = load4f (by [Y [P [2 * n + 1]]]);         //     \|/  \|/     //
    v4f z1 = load4f (cz [Z [P [2 * n + 1]]]);         //      Y1--Z1      //
                                                      //       \  /       //
    if (chirality == 1) {                             //        X3        //
      v4f x2 = load4f (ax [s [4 * n + 1]]);
      v4f x4 = load4f (ax [s [4 * n + 3]]);           //                               ^           //
      a = x2 + y1 + z0;                               //                              / \          //
      b = x0 + y1 + z1;                               //                             / A \         //
      c = x0 + y0 + z0;                               //        +-----+             +-----+        //
      d = x4 + y0 + z1;                               //       /|\ C /|\            |\   /|        //
    }                                                 //      / | \ / | \           | \ / |        //
    else {                                            //     < A|  x  |D >          |B x C|        //
      v4f x1 = load4f (ax [s [4 * n + 0]]);           //      \ | / \ | /           | / \ |        //
      v4f x3 = load4f (ax [s [4 * n + 2]]);           //       \|/ B \|/            |/   \|        //
      a = x1 + y0 + z0;                               //        +-----+             +-----+        //
      b = x0 + y1 + z0;                               //                             \ D /         //
      c = x0 + y0 + z1;                               //                              \ /          //
      d = x3 + y1 + z1;                               //                               v           //
    }                                                 //                                           //

    float out [8] [4] ALIGNED16;
    store4f (out [0], a + b + c); // The normal for triangle ABC.
    store4f (out [1], d + c + b); // The normal for triangle DCB.
    store4f (out [2], a);
    store4f (out [3], b);
    store4f (out [4], c);
    store4f (out [5], d);

    glBegin (GL_TRIANGLES);
    glNormal3fv (out [0]);
    glVertex3fv (out [2]);
    glVertex3fv (out [3]);
    glVertex3fv (out [4]);
    glNormal3fv (out [1]);
    glVertex3fv (out [5]);
    glVertex3fv (out [4]);
    glVertex3fv (out [3]);
    glEnd ();
  }
}

void
paint (const object_t & object, const float (& f) [16])
{
  GLfloat rgb [3] [4]; // [ambient, diffuse, specular] [R, G, B, A]

  float h = object.hue;
  float s = object.saturation;
  float v = object.intensity;

  for (unsigned k = 0; k != 3; ++ k)
  {
    float i = h < 2.0 ? 1.0 : h < 3.0 ? 3.0 - h : h < 5.0 ? 0.0 : h - 5.0;
    rgb [0] [k] = 0.01f;                  // ambient
    rgb [1] [k] = v * (s + i * (1 - s));  // diffuse
    rgb [2] [k] = 0.3f + 0.7f * v;        // specular
    h = h < 2.0 ? h + 4.0 : h - 2.0;
  }

  rgb [0] [3] = 1.0f;
  rgb [1] [3] = 1.0f;
  rgb [2] [3] = 1.0f;

  glMatrixMode (GL_MODELVIEW);
  glLoadMatrixf (f);
  glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, rgb [0]);
  glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, rgb [1]);
  glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, rgb [2]);
  if (object.display_list) glCallList (object.display_list);
  else object.system_ref->paint (object);
}
