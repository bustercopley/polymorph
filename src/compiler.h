// -*- C++ -*-
#ifndef compiler_h
#define compiler_h

#ifdef __GNUC__
#define NOINLINE __attribute__ ((noinline))
#else
#define NOINLINE
#endif

#endif
