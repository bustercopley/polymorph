// -*- C++ -*-

#ifndef compiler_h
#define compiler_h

#if defined (__GNUC__)
#define NOINLINE __attribute__ ((noinline))
#define ALWAYS_INLINE __attribute__ ((always_inline))
#define ALIGNED16 __attribute__ ((aligned (16)))
#define VISIBLE __attribute__ ((externally_visible))
#define ALIGN_STACK __attribute__ ((force_align_arg_pointer))
#define NORETURN __attribute__ ((noreturn))
#define RESTRICT __restrict__
#elif defined (_MSC_VER)
#define NOINLINE
#define ALWAYS_INLINE
#define ALIGNED16 __declspec (align (16))
#define VISIBLE
#define ALIGN_STACK
#define NORETURN __declspec (noreturn)
#define RESTRICT __restrict
#else
#define NOINLINE
#define ALWAYS_INLINE
#define ALIGNED16
#define VISIBLE
#define ALIGN_STACK
#define NORETURN
#define RESTRICT
#endif

#endif
