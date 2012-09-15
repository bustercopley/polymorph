// -*- C++ -*-
#ifndef compiler_h
#define compiler_h

#ifdef __GNUC__
#define NOINLINE __attribute__ ((noinline))
#define ALIGNED16 __attribute__ ((aligned(16)))
#else
#define NOINLINE
#define ALIGNED16
#endif

#endif
