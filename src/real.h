// -*- C++ -*-

#ifndef real_h
#define real_h

#define USE_FLOAT
#ifdef USE_FLOAT

typedef float real;
#define GLreal GLfloat
#define glVertex3rv glVertex3fv
#define glNormal3rv glNormal3fv
#define glLoadMatrixr glLoadMatrixf

#else

typedef double real;
#define GLreal GLdouble
#define glVertex3rv glVertex3dv
#define glNormal3rv glNormal3dv
#define glLoadMatrixr glLoadMatrixd

#endif
#endif
