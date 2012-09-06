#include <GL/gl.h>
#include "graphics.h"
#include "object.h"
#include "system_ref.h"
#include "frustum.h"
#include "vector.h"
#include "maths.h"

int get_lists_start (unsigned n) {
  return glGenLists (n);
}

void begin_list (int n) {
  glNewList (n, GL_COMPILE);
}

void end_list () {
  glEndList ();
}

void lights (real distance, real depth, real lnear, real lfar, real dnear) {
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  GLfloat lights [5] [4] = {
    { 0.00f, 0.00f, 0.00f, 1.00f, }, // Position.
    { 0.42f, 0.42f, 0.42f, 0.00f, }, // Ambient intensity.
    { 0.80f, 0.80f, 0.80f, 0.00f, }, // Diffuse intensity.
    { 0.40f, 0.40f, 0.40f, 0.00f, }, // Specular intensity.
  };

  lights [0] [2] = -distance + dnear;

  glLightfv (GL_LIGHT0, GL_POSITION, lights [0]);
  glLightfv (GL_LIGHT0, GL_AMBIENT, lights [1]);
  glLightfv (GL_LIGHT0, GL_DIFFUSE, lights [2]);
  glLightfv (GL_LIGHT0, GL_SPECULAR, lights [3]);

  real dfar = dnear + depth;

  glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION, ((dfar / lnear) - (dnear / lfar)) / (dfar - dnear));
  glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION, ((1 / lfar) - (1 / lnear)) / (dfar - dnear));

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
  float x1 = view.width / 2;
  float y1 = view.height / 2;
  float z1 = view.distance;
  float z2 = view.distance + view.depth;
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  // glFrustum's z arguments are specified as positive distances,
  // but the z co-ordinate of a point inside the viewing frustum
  // is negative (in eye co-ordinates).
  glFrustum (-x1, x1, -y1, y1, z1 + 0.05f, z2);
}

void clear () {
  glClear (GL_COLOR_BUFFER_BIT);
}

namespace {
  inline void
  point (const real (& au) [3], const real (& bv) [3], const real (& cw) [3]) {
    // Send a vertex `p' to the graphics library. `p' is the sum of `au', `bv' and `cw'.

    // The vector `au' is the product of a scalar `a' and a vector 'u', and so on.
    // In other words, `a', `b', `c' are the co-ordinates of `p' in the co-ordinate
    // system defined by the basis (`u', `v', `w').

    // `u', `v' and `w' are nodes of a Moebius triangle in the spherical tiling.
    // `a', `b' and `c' have been chosen so that `p' lies on the unit sphere.
    // Certain values of the co-ordinates `a', `b', `c' pick out vertices of
    // uniform polyhedra: see "nodes/make_system.tcc" for details.
    GLreal p [3];
    add (au, bv, cw, p);
    glVertex3rv (p);
  }
}

void
paint_kpgons (unsigned k, unsigned Np, unsigned p,
              const unsigned char * x, const real (* u) [3],
              const real (* au) [3], const real (* bv) [3], const real (* cw) [3]) {
  if (k == 1) {
    for (unsigned n = 0; n != Np; ++ n) {
      glBegin (GL_POLYGON);
      glNormal3rv (u [n]);
      const real (& v) [3] = bv [x [n * (2 * p) + 0]];
      const real (& w) [3] = cw [x [n * (2 * p) + (2 * p) - 1]];
      point (au [n], v, w);
      for (unsigned k0 = 1; k0 != p; ++ k0) {
        const real (& v) [3] = bv [x [n * (2 * p) + (2 * k0) + 0]];
        const real (& w) [3] = cw [x [n * (2 * p) + (2 * k0) - 1]];
        point (au [n], v, w);
      }
      glEnd ();
    }
  }
  else if (k == 2) {
    for (unsigned n = 0; n != Np; ++ n) {
      glBegin (GL_POLYGON);
      glNormal3rv (u [n]);
      const real (* w) [3] = & cw [x [n * (2 * p) + 2 * p - 1]];
      for (unsigned k0 = 0; k0 != p; ++ k0) {
        const real (* v) [3] = & bv [x [n * (2 * p) + (2 * k0) + 0]];
        point (au [n], * v, * w);
        w = & cw [x [n * (2 * p) + (2 * k0) + 1]];
        point (au [n], * v, * w);
      }
      glEnd ();
    }
  }
}

void
paint_snub_pgons (int chirality, unsigned Np, unsigned p,
                  const unsigned char * x,
                  const real (* u) [3],
                  const real (* au) [3], const real (* bv) [3], const real (* cw) [3]) {
  for (unsigned n = 0; n != Np; ++ n) {
    glBegin (GL_POLYGON);
    glNormal3rv (u [n]);
    if (chirality == 1) {
      // By considering the p black tiles around each p-node,
      // obtain N/p p-gonal (or degenerate) faces.
      for (unsigned k = 0; k != p; ++ k)
        point (au [n],
               bv [x [2 * n * p + 2 * k]],
               cw [x [2 * n * p + 2 * k + 1]]);
    }
    else {
      // The same, but with the white tiles.
      point (au [n],
             bv [x [2 * n * p]],
             cw [x [2 * n * p + 2 * p - 1]]);
      for (unsigned k = 1; k != p; ++ k)
        point (au [n],
               bv [x [2 * n * p + 2 * k]],
               cw [x [2 * n * p + 2 * k - 1]]);
    }
    glEnd ();
  }
}

void
paint_snub_triangle_pairs (int chirality, unsigned Np,
                           const unsigned char * x,
                           const unsigned char (* s) [4],
                           const real (* au) [3], const real (* bv) [3], const real (* cw) [3]) {
  for (unsigned n = 0; n != Np; ++ n) {          //        X1        //
    const real (& x0) [3] = au [n];              //       /  \       //
    const real (& y0) [3] = bv [x [4 * n + 0]];  //      Z0--Y0      //
    const real (& z0) [3] = cw [x [4 * n + 1]];  //     /|\  /|\     //
    const real (& y1) [3] = bv [x [4 * n + 2]];  //   X4 | X0 | X2   //
    const real (& z1) [3] = cw [x [4 * n + 3]];  //     \|/  \|/     //
    real a [3], b [3], c [3], d [3];             //      Y1--Z1      //
                                                 //       \  /       //
    if (chirality == 1) {                        //        X3        //
      const real (& x2) [3] = au [s [n] [1]];
      const real (& x4) [3] = au [s [n] [3]];    //                               ^           //
      add (x2, y1, z0, a);                       //                              / \          //
      add (x0, y1, z1, b);                       //                             / A \         //
      add (x0, y0, z0, c);                       //        +-----+             +-----+        //
      add (x4, y0, z1, d);                       //       /|\ C /|\            |\   /|        //
    }                                            //      / | \ / | \           | \ / |        //
    else {                                       //     < A|  x  |D >          |B x C|        //
      const real (& x1) [3] = au [s [n] [0]];    //      \ | / \ | /           | / \ |        //
      const real (& x3) [3] = au [s [n] [2]];    //       \|/ B \|/            |/   \|        //
      add (x1, y0, z0, a);                       //        +-----+             +-----+        //
      add (x0, y1, z0, b);                       //                             \ D /         //
      add (x0, y0, z1, c);                       //                              \ /          //
      add (x3, y1, z1, d);                       //                               v           //
    }                                            //                                           //
                                                 //      chirality=1          chirality=-1    //
    real n0 [3], n1 [3];
    add (a, b, c, n0);
    add (d, c, b, n1);
    glBegin (GL_TRIANGLES);
    glNormal3rv (n0);
    glVertex3rv (a);
    glVertex3rv (b);
    glVertex3rv (c);
    glNormal3rv (n1);
    glVertex3rv (d);
    glVertex3rv (c);
    glVertex3rv (b);
    glEnd ();
  }
}

void
paint (const object_t & object, const float (& f) [16])
{
  GLfloat rgb [3] [4]; // [ambient, diffuse, specular] [R, G, B, A]

  real h = object.hue;
  real s = object.saturation;
  real v = object.intensity;

  for (unsigned k = 0; k != 3; ++ k)
  {
    real i = h < 2.0 ? 1.0 : h < 3.0 ? 3.0 - h : h < 5.0 ? 0.0 : h - 5.0;
    rgb [0] [k] = 0.01f;                  // ambient
    rgb [1] [k] = v * (s + i * (1 - s));  // diffuse
    rgb [2] [k] = 0.3f + 0.7f * v;        // specular
    h = h < 2.0 ? h + 4.0 : h - 2.0;
  }

  rgb [0] [3] = 1.0f;
  rgb [1] [3] = 1.0f;
  rgb [2] [3] = 1.0f;

  glMatrixMode (GL_MODELVIEW);
  glLoadMatrixr (f);
  glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, rgb [0]);
  glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, rgb [1]);
  glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, rgb [2]);
  if (object.display_list) glCallList (object.display_list);
  else object.system_ref->paint (object);
}
