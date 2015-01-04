// -*- C++ -*-

#ifndef rotor_h
#define rotor_h

#include "rodrigues.h"

struct rotor_t
{
  rotor_t (const float (& u) [4], float A);
  void operator () (const float (& in) [4], float (& out) [4]) const;
private:
  float matrix [4] [4];
};

inline rotor_t::rotor_t (const float (& u) [4], float a)
{
  ALIGNED16 float X [4] = { 0.0f, 0.0f, 0.0f, 0.0f, };
  ALIGNED16 float U [4];
  store4f (U, _mm_set1_ps (a) * _mm_load_ps (u));
  unsigned dummy_order = 0;
  compute ((char *) & matrix, 0, & X, & U, & dummy_order, 1);
}

inline void rotor_t::operator () (const float (& in) [4], float (& out) [4]) const
{
  store4f (out, tmapply (matrix, load4f (in)));
}

#endif
