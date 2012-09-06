// -*- C++ -*-

#ifndef kdtree_h
#define kdtree_h

struct kdtree_t
{
  kdtree_t () : memory (nullptr), node_count (0) { }
  ~kdtree_t ();
  void compute (unsigned * index, const float (* x) [4], unsigned count);
  typedef void (* callback_t) (void * data, unsigned i, unsigned j);
  void for_near (unsigned count, float r, void * data, callback_t f);
  void for_near (float (* planes) [2] [4], float r, void * data, callback_t f);
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
