// -*- C++ -*-

#ifndef qpc_h
#define qpc_h

#include "mswin.h"

inline std::uint64_t qpc ()
{
  LARGE_INTEGER qpc;
  ::QueryPerformanceCounter (& qpc);
  return qpc.QuadPart;
}

#endif
