#include "snub_variance.h"

namespace
{
  struct variance
  {
    // This algorithm from TAOCP by Donald E. Knuth, citing B.P. Welford;
    // see `http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance'.

    variance () : n (0), mean (0), S (0) { }

    variance & operator << (long double x)
    {
      ++ n;
      long double delta = x - mean;
      mean += delta / n;
      S += delta * (x - mean);
      return * this;
    }

    long double operator () ()
    {
      return S / (n - 1);
    }

  private:
    unsigned n;
    long double mean;
    long double S;
  };

  inline long double dot (const long double (& u) [3], const long double (& v) [3])
  {
    return u [0] * v [0] + u [1] * v [1] + u [2] * v [2];
  }
}

long double snub_variance (const uint8_t * P, const uint8_t * Q, const uint8_t * R,
                           const uint8_t * X, const uint8_t * Y, const uint8_t * Z,
                           const float (* x) [3], const float (* y) [3], const float (* z) [3],
                           const float (& g7) [3], unsigned N)
{
  variance Var;

  for (unsigned n = 0; n != N; ++ n) {
    unsigned i = n;
    unsigned j = i [R];
    unsigned k = j [P];
    if (i != k [Q]) return -1.0;

    const float (& X0) [3] = x [X [k]];
    const float (& Y0) [3] = y [Y [i]];
    const float (& Z0) [3] = z [Z [j]];
    const float (& X1) [3] = x [X [i]];
    const float (& Y1) [3] = y [Y [j]];
    const float (& Z1) [3] = z [Z [k]];

    long double alpha = g7 [0];
    long double beta  = g7 [1];
    long double gamma = g7 [2];

    long double U [3], V [3], W [3];

    for (unsigned i = 0; i != 3; ++ i) {
      U [i] = alpha * X1 [i] + beta * Y0 [i] + gamma * Z0 [i];
      V [i] = alpha * X0 [i] + beta * Y1 [i] + gamma * Z0 [i];
      W [i] = alpha * X0 [i] + beta * Y0 [i] + gamma * Z1 [i];
    }

    long double VW = dot (V, W);
    long double WU = dot (W, U);
    long double UV = dot (U, V);

    Var << VW << WU << UV;
  }

  return Var ();
}
