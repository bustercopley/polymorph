// -*- C++ -*-

#ifndef kdtree_h
#define kdtree_h

struct object_t;

struct kdtree_t
{
  kdtree_t () : memory (nullptr), node_count (0) { }
  ~kdtree_t ();
  void compute (unsigned * index, const float (* x) [4], unsigned count);
  void bounce (unsigned count, float max_radius,
               object_t * objects,
               float (* v) [4], float (* w) [4],
               float (* planes) [2] [4]);
private:
  unsigned * index;
  const float (* x) [4];
  void * memory;
  float (* node_lohi) [2] [4];
  unsigned * node_begin;
  unsigned * node_end;
  unsigned node_count;
};

#endif
