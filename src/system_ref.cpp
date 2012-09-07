#include "system_ref.h"
#include "nodes/system.h"
#include "object.h"
#include "graphics.h"
#include "vector.h"
#include "maths.h"

void system_ref_t::paint (object_t const & object) const
{
  float const (& A) [3] = g [object.locus_begin];
  float const (& B) [3] = g [object.locus_end];

  float alpha = object.generator_position;
  float one_minus_alpha = 1 - alpha;
  float abc [3], generator [3];

  abc [0] = one_minus_alpha * A [0] + alpha * B [0];
  abc [1] = one_minus_alpha * A [1] + alpha * B [1];
  abc [2] = one_minus_alpha * A [2] + alpha * B [2];

  generator [0] = abc [0] * x [0] [0] + abc [1] * y [0] [0] + abc [2] * z [0] [0];
  generator [1] = abc [0] * x [0] [1] + abc [1] * y [0] [1] + abc [2] * z [0] [1];
  generator [2] = abc [0] * x [0] [2] + abc [1] * y [0] [2] + abc [2] * z [0] [2];

  float adjust (std::pow (generator [0] * generator [0] +
                          generator [1] * generator [1] +
                          generator [2] * generator [2], -0.5f));

  for (unsigned n = 0; n != N / p; ++ n) {
    float a = abc [0] * adjust;
    scalar_multiply (a, x [n], ax [n]);
  }

  for (unsigned n = 0; n != N / q; ++ n) {
    float b = abc [1] * adjust;
    scalar_multiply (b, y [n], by [n]);
  }

  for (unsigned n = 0; n != N / r; ++ n) {
    float c = abc [2] * adjust;
    scalar_multiply (c, z [n], cz [n]);
  }

  if (object.chirality) {
    paint_snub_triangle_pairs (object.chirality, N / p, P, s, Y, Z, ax, by, cz);
    paint_snub_pgons (object.chirality, N / q, q, Q, Z, X, y, by, cz, ax);
    paint_snub_pgons (object.chirality, N / r, r, R, X, Y, z, cz, ax, by);
  }
  else {
    paint_kpgons (2, N / p, p, P, Y, Z, x, ax, by, cz);
    paint_kpgons (2, N / q, q, Q, Z, X, y, by, cz, ax);
    paint_kpgons (2, N / r, r, R, X, Y, z, cz, ax, by);
  }
}

void system_ref_t::save_display_lists (int list)
{
  lists_start = list;
  for (unsigned i = 0; i != 9; ++ i) {
    unsigned generator;
    int chirality;

    if (i == 8) {
      generator = 7;
      chirality = -1;
    }
    else if (i == 7) {
      generator = 7;
      chirality = +1;
    }
    else {
      generator = i;
      chirality = 0;
    }

    float const (& abc) [3] = g [generator];

    for (unsigned n = 0; n != N / p; ++ n) scalar_multiply (abc [0], x [n], ax [n]);
    for (unsigned n = 0; n != N / q; ++ n) scalar_multiply (abc [1], y [n], by [n]);
    for (unsigned n = 0; n != N / r; ++ n) scalar_multiply (abc [2], z [n], cz [n]);

    begin_list (lists_start + i);

    if (chirality) {
      paint_snub_triangle_pairs (chirality, N / p, P, s, Y, Z, ax, by, cz);
      paint_snub_pgons (chirality, N / q, q, Q, Z, X, y, by, cz, ax);
      paint_snub_pgons (chirality, N / r, r, R, X, Y, z, cz, ax, by);
    }
    else {
      const unsigned pk [7] = { 0, 0, 0, 2, 0, 0, 2, };
      const unsigned qk [7] = { 1, 0, 1, 1, 2, 1, 2, };
      const unsigned rk [7] = { 1, 1, 0, 1, 1, 2, 2, };
      paint_kpgons (pk [generator], N / p, p, P, Y, Z, x, ax, by, cz);
      paint_kpgons (qk [generator], N / q, q, Q, Z, X, y, by, cz, ax);
      paint_kpgons (rk [generator], N / r, r, R, X, Y, z, cz, ax, by);
    }
    end_list ();
  }
}

system_ref_t const * system_repository_t::ref (system_select_t select) const
{
  if (select == tetrahedral) {
    return & T;
  }
  else if (select == octahedral) {
    return & O;
  }
  else if (select == icosahedral) {
    return & I;
  }
  else {
    return 0;
  }
}

template <unsigned q, unsigned r>
void initialize_system (system_ref_t & system_ref, const system_t <q, r> & system, float (* scratch) [3])
{
  enum { p = 2 };
  system_ref.p = p;
  system_ref.q = q;
  system_ref.r = r;
  system_ref.N = system.N;
  system_ref.x = system.x;
  system_ref.y = system.y;
  system_ref.z = system.z;
  system_ref.g = system.g;
  system_ref.ax = scratch;
  system_ref.by = scratch + system.N / p;
  system_ref.cz = scratch + system.N / p + system.N / q;
  system_ref.P = * system.P;
  system_ref.Q = * system.Q;
  system_ref.R = * system.R;
  system_ref.X = system.X;
  system_ref.Y = system.Y;
  system_ref.Z = system.Z;
  system_ref.s = * system.s;
}

void system_repository_t::initialize (void * data)
{
  system_t <3, 3> const & t (* reinterpret_cast <system_t <3, 3> const *> (data));
  system_t <3, 4> const & o (* reinterpret_cast <system_t <3, 4> const *> (& t + 1));
  system_t <3, 5> const & i (* reinterpret_cast <system_t <3, 5> const *> (& o + 1));
  // <233> N = 12; nT = N/p + N/q + N/r = 6 + 4 + 4 = 14 = N + 2 (Euler characteristic is 2)
  // <234> N = 24; nO = N/p + N/q + N/r = 12 + 8 + 6 = 26 = N + 2
  // <235> N = 60; nI = N/p + N/q + N/r = 30 + 20 + 12 = 62 = N + 2
  // nT + nO + nI = 102.
  initialize_system (T, t, scratch);
  initialize_system (O, o, scratch + t.N + 2);
  initialize_system (I, i, scratch + t.N + 2 + o.N + 2);

  lists_start = get_lists_start (27); // error handling?
  T.save_display_lists (lists_start + 0 * 9);
  O.save_display_lists (lists_start + 1 * 9);
  I.save_display_lists (lists_start + 2 * 9);
}
