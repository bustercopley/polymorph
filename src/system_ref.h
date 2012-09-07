// -*- C++ -*-

#ifndef system_ref_h
#define system_ref_h

#include <cstdint>

struct object_t;

enum system_select_t
{
  tetrahedral, octahedral, icosahedral,
  hexahedral = octahedral,
  dodecahedral = icosahedral
};

struct system_ref_t
{
  void paint (object_t const & object) const;
  void save_display_lists (int list);
  unsigned N, p, q, r;
  uint8_t const * P, * Q, * R, * X, * Y, * Z, * s;
  float const (* x) [3], (* y) [3], (* z) [3];
  float const (* g) [3];
  float (* ax) [3], (* by) [3], (* cz) [3];
  int lists_start;
};

struct system_repository_t
{
  system_ref_t const * ref (system_select_t system_select) const;
  void initialize (void * data);
private:
  system_ref_t T, O, I;
  int lists_start;

  // <pqr>
  // <233> (N = 12): nT = N/p + N/q + N/r = 6 + 4 + 4 = 14
  // <234> (N = 24): nO = N/p + N/q + N/r = 12 + 8 + 6 = 26
  // <235> (N = 60): nI = N/p + N/q + N/r = 30 + 20 + 12 = 62
  // nT + nO + nI = 102;

  float scratch [102] [3];
};

#endif
