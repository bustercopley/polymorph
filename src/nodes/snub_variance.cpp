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

long double snub_variance (const float (* u) [3], const float (* v) [3], const float (* w) [3],
                           const float (& g7) [3], unsigned Np, const uint8_t * x, const uint8_t (* s) [4])
{
  variance Var;

  for (unsigned n = 0; n != Np; ++ n) {
    const float (& Xn) [3] = u [n];
    const float (& Y0) [3] = v [x [4 * n + 0]];
    const float (& Z0) [3] = w [x [4 * n + 1]];
    const float (& Y1) [3] = v [x [4 * n + 2]];
    const float (& Z1) [3] = w [x [4 * n + 3]];
    const float (& X0) [3] = u [s [n] [0]];
    const float (& X1) [3] = u [s [n] [1]];
    const float (& X2) [3] = u [s [n] [2]];
    const float (& X3) [3] = u [s [n] [3]];

    long double alpha = g7 [0];
    long double beta  = g7 [1];
    long double gamma = g7 [2];

    long double P [3], Q [3], R [3], S [3], T [3], U [3], V [3], W [3];

    for (unsigned i = 0; i != 3; ++ i) {
      P [i] = alpha * X0 [i] + beta * Y0 [i] + gamma * Z0 [i];
      Q [i] = alpha * Xn [i] + beta * Y1 [i] + gamma * Z0 [i];
      R [i] = alpha * Xn [i] + beta * Y0 [i] + gamma * Z1 [i];
      S [i] = alpha * X2 [i] + beta * Y1 [i] + gamma * Z1 [i];
      T [i] = alpha * X1 [i] + beta * Y1 [i] + gamma * Z0 [i];
      U [i] = alpha * Xn [i] + beta * Y1 [i] + gamma * Z1 [i];
      V [i] = alpha * Xn [i] + beta * Y0 [i] + gamma * Z0 [i];
      W [i] = alpha * X3 [i] + beta * Y0 [i] + gamma * Z1 [i];
    }

    long double QP = dot (Q, P);
    long double PR = dot (P, R);
    long double RQ = dot (R, Q);
    long double QS = dot (Q, S);
    long double SR = dot (S, R);

    long double UT = dot (U, T);
    long double TV = dot (T, V);
    long double VU = dot (V, U);
    long double UW = dot (U, W);
    long double WV = dot (W, V);

    Var << QP << PR << RQ << QS << SR << UT << TV << VU << UW << WV;
  }

  return Var ();
}
