// -*- C++ -*-

#ifndef snub_variance_h
#define snub_variance_h

#include "../real.h"

long double snub_variance (const real (* u) [3], const real (* v) [3], const real (* w) [3],
                           const real (& k7) [3], unsigned Np, const unsigned char * x, const unsigned char (* s) [4]);

#endif
