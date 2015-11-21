// -*- C++ -*-

#ifndef glcheck_h
#define glcheck_h

//#define ENABLE_GLCHECK
#define GLCHECK_TRACE 0

#ifdef ENABLE_GLCHECK
#define GLCHECK_ENABLED 1
#else
#define GLCHECK_ENABLED 0
#endif

#ifdef ENABLE_GLDEBUG
#define GLDEBUG_ENABLED 1
#else
#define GLDEBUG_ENABLED 0
#endif

#endif
