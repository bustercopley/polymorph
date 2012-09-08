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
  float const (* x) [4], (* y) [4], (* z) [4];
  float const (* g) [4];
  float (* ax) [4], (* by) [4], (* cz) [4];
  int lists_start;
};

struct system_repository_t
{
  system_repository_t () : memory (nullptr) { }
  ~system_repository_t ();
  system_ref_t const * ref (system_select_t system_select) const;
  void initialize (void * data);
private:
  void * memory;
  system_ref_t T, O, I;
  int lists_start;
};

#endif
