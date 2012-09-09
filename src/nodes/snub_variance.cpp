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

long double snub_variance (const uint8_t * P, const uint8_t * Y, const uint8_t * Z,
                           const float (* x) [3], const float (* y) [3], const float (* z) [3],
                           const float (& g7) [3], unsigned N, unsigned p,
                           const uint8_t (* s) [4])
{
  variance Var;

  for (unsigned n = 0; n != N / p; ++ n) {
    const float (& Xn) [3] = x [n];
    const float (& Y0) [3] = y [Y [P [p * n + 0]]];
    const float (& Z0) [3] = z [Z [P [p * n + 0]]];
    const float (& Y1) [3] = y [Y [P [p * n + 1]]];
    const float (& Z1) [3] = z [Z [P [p * n + 1]]];
    const float (& X0) [3] = x [s [n] [0]];
    const float (& X1) [3] = x [s [n] [1]];
    const float (& X2) [3] = x [s [n] [2]];
    const float (& X3) [3] = x [s [n] [3]];

    long double alpha = g7 [0];
    long double beta  = g7 [1];
    long double gamma = g7 [2];

    long double A [3], B [3], C [3], D [3], E [3], F [3], G [3], H [3];

    for (unsigned i = 0; i != 3; ++ i) {
      A [i] = alpha * X0 [i] + beta * Y0 [i] + gamma * Z0 [i];
      B [i] = alpha * Xn [i] + beta * Y1 [i] + gamma * Z0 [i];
      C [i] = alpha * Xn [i] + beta * Y0 [i] + gamma * Z1 [i];
      D [i] = alpha * X2 [i] + beta * Y1 [i] + gamma * Z1 [i];
      E [i] = alpha * X1 [i] + beta * Y1 [i] + gamma * Z0 [i];
      F [i] = alpha * Xn [i] + beta * Y1 [i] + gamma * Z1 [i];
      G [i] = alpha * Xn [i] + beta * Y0 [i] + gamma * Z0 [i];
      H [i] = alpha * X3 [i] + beta * Y0 [i] + gamma * Z1 [i];
    }

    long double BA = dot (B, A);
    long double AC = dot (A, C);
    long double CB = dot (C, B);
    long double BD = dot (B, D);
    long double DC = dot (D, C);

    long double FE = dot (F, E);
    long double EG = dot (E, G);
    long double GF = dot (G, F);
    long double FH = dot (F, H);
    long double HG = dot (H, G);

    Var << BA << AC << CB << BD << DC << FE << EG << GF << FH << HG;
  }

  return Var ();
}
