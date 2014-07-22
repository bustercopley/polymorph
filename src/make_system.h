// -*- C++ -*-

#ifndef make_system_h
#define make_system_h

struct system_data_t
{
  float (* xyz) [4];
  unsigned (* indices) [6];
};

system_data_t make_system (unsigned q, unsigned r, const float (& xyz_in) [3] [4], char * memory);

#endif
