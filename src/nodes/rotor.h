// -*- C++ -*-

#ifndef rotor_h
#define rotor_h

struct rotor_t {
  rotor_t (long double angle);
  void about (long double (& a) [3]); // a must be a unit vector.
  void operator () (const long double (& in) [3], long double (& out) [3]) const;
private:
  long double sinA, cosA;
  long double matrix [3] [3];
};

#endif
