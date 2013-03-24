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

    unsigned n;
    long double mean;
    long double S;
  };

  inline long double dot (const long double (& u) [4], const long double (& v) [4])
  {
    return u [0] * v [0] + u [1] * v [1] + u [2] * v [2];
  }
}

std::pair <double, double>
snub_variance (const float (* xyz) [4],
               const std::uint8_t (* indices) [6],
               const float (& g7) [4], unsigned N)
{
  variance Var;

  for (unsigned n = 0; n != N; ++ n) {

    const float (& X0) [4] = xyz [indices [n] [0]];
    const float (& Y1) [4] = xyz [indices [n] [1]];
    const float (& Z0) [4] = xyz [indices [n] [2]];
    const float (& X1) [4] = xyz [indices [n] [3]];
    const float (& Y0) [4] = xyz [indices [n] [4]];
    const float (& Z1) [4] = xyz [indices [n] [5]];

    long double alpha = g7 [0];
    long double beta  = g7 [1];
    long double gamma = g7 [2];

    long double U [4], V [4], W [4];

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

  return std::make_pair (Var.mean, Var ());
}
