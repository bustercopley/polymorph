// -*- C++ -*-

#ifndef graphics_h
#define graphics_h

#include "real.h"
#include "frustum.h"

struct object_t;

void paint (const object_t & object, const float (& f) [16]);
int get_lists_start (unsigned n);
void begin_list (int n);
void end_list ();

void screen (int width, int height);
void box (const view_t & view);
void lights (real znear, real depth, real lnear, real lfar, real dnear);
void clear ();

void
paint_snub_pgons (int chirality, unsigned Np, unsigned p,
        const unsigned char * x,
        const real (* u) [3],
        const real (* au) [3],
        const real (* bv) [3],
        const real (* cw) [3]);

void
paint_snub_triangle_pairs (int chirality, unsigned Np,
        const unsigned char * x,
        const unsigned char (* s) [4],
        const real (* au) [3], const real (* bv) [3], const real (* cw) [3]);

void
paint_kpgons (unsigned k, unsigned Np, unsigned p,
              const unsigned char * x, const real (* u) [3],
              const real (* au) [3], const real (* bv) [3], const real (* cw) [3]);

#endif
