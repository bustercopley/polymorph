// -*- C++ -*-

#ifndef compiler_h
#define compiler_h

#ifdef __GNUC__
#define NOINLINE __attribute__ ((noinline))
#define ALWAYS_INLINE __attribute__ ((always_inline))
#define ALIGNED16 __attribute__ ((aligned(16)))
#define RESTRICT __restrict__
#else
#define NOINLINE
#define ALWAYS_INLINE
#define ALIGNED16
#define RESTRICT
#endif

#endif
