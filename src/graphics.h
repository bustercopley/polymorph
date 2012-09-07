// -*- C++ -*-

#ifndef graphics_h
#define graphics_h

#include "frustum.h"

struct object_t;

void paint (const object_t & object, const float (& f) [16]);
int get_lists_start (unsigned n);
void begin_list (int n);
void end_list ();

void screen (int width, int height);
void box (const view_t & view);
void lights (float znear, float depth, float lnear, float lfar, float dnear);
void clear ();

void paint_kpgons (unsigned k, unsigned Np, unsigned p,
                   const uint8_t * P,
                   const uint8_t * Y, const uint8_t * Z,
                   const float (* x) [3],
                   const float (* ax) [3], const float (* by) [3], const float (* cz) [3]);

void paint_snub_pgons (int chirality, unsigned Np, unsigned p,
                       const uint8_t * P,
                       const uint8_t * Y, const uint8_t * Z,
                       const float (* x) [3],
                       const float (* ax) [3], const float (* by) [3], const float (* cz) [3]);

void paint_snub_triangle_pairs (int chirality, unsigned Np,
                                const uint8_t * P, const uint8_t * s,
                                const uint8_t * Y, const uint8_t * Z,
                                const float (* ax) [3], const float (* by) [3], const float (* cz) [3]);

#endif
