// -*- C++ -*-

#ifndef graphics_h
#define graphics_h

#include "frustum.h"
#include "object.h"
#include <cstdint>

void paint (const object_t & object, const float (& f) [16]);
int get_lists_start (unsigned n);
void begin_list (int n);
void end_list ();

void screen (int width, int height);
void box (const view_t & view);
void lights (float znear, float depth, float lnear, float lfar, float dnear);
void clear ();

void paint_kpgons (unsigned k, unsigned Np, unsigned p,
                   const std::uint8_t * P,
                   const std::uint8_t * Y, const std::uint8_t * Z,
                   const float (* x) [4],
                   const float (* ax) [4], const float (* by) [4], const float (* cz) [4]);

void paint_snub_pgons (int chirality, unsigned Np, unsigned p,
                       const std::uint8_t * P,
                       const std::uint8_t * Y, const std::uint8_t * Z,
                       const float (* x) [4],
                       const float (* ax) [4], const float (* by) [4], const float (* cz) [4]);

void paint_snub_triangle_pairs (int chirality, unsigned Np,
                                const std::uint8_t * P, const std::uint8_t * s,
                                const std::uint8_t * Y, const std::uint8_t * Z,
                                const float (* ax) [4], const float (* by) [4], const float (* cz) [4]);

#endif
