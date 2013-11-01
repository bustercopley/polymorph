#include "rotor.h"
#include <cmath>

rotor_t::rotor_t (long double A) : sinA (std::sin (A)), cosA (std::cos (A)) { }

void rotor_t::about (const long double (& a) [4])
{
  // Store the matrix of the rotation about `a' through angle `A'.
  long double symm12 = a [1] * a [2] * (1 - cosA);
  long double symm20 = a [2] * a [0] * (1 - cosA);
  long double symm01 = a [0] * a [1] * (1 - cosA);

  long double skew0 = sinA * a [0];
  long double skew1 = sinA * a [1];
  long double skew2 = sinA * a [2];

  matrix [0] [0] = cosA + a [0] * a [0] * (1 - cosA);
  matrix [0] [1] = symm01 - skew2;
  matrix [0] [2] = symm20 + skew1;

  matrix [1] [0] = symm01 + skew2;
  matrix [1] [1] = cosA + a [1] * a [1] * (1 - cosA);
  matrix [1] [2] = symm12 - skew0;

  matrix [2] [0] = symm20 - skew1;
  matrix [2] [1] = symm12 + skew0;
  matrix [2] [2] = cosA + a [2] * a [2] * (1 - cosA);
}

void rotor_t::operator () (const long double (& in) [4], long double (& out) [4]) const
{
  // Apply the rotation to the vector `in' and store the result in `out'.
  out [0] = matrix [0] [0] * in [0] + matrix [0] [1] * in [1] + matrix [0] [2] * in [2];
  out [1] = matrix [1] [0] * in [0] + matrix [1] [1] * in [1] + matrix [1] [2] * in [2];
  out [2] = matrix [2] [0] * in [0] + matrix [2] [1] * in [1] + matrix [2] [2] * in [2];
}
