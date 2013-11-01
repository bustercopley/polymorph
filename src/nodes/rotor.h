// -*- C++ -*-

#ifndef rotor_h
#define rotor_h

struct rotor_t
{
  rotor_t (long double angle);
  void about (const long double (& a) [4]); // a must be a unit vector.
  void operator () (const long double (& in) [4], long double (& out) [4]) const;
private:
  const long double sinA, cosA;
  long double matrix [4] [4];
};

#endif
