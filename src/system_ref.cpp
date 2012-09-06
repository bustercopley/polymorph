#include "system_ref.h"
#include "nodes/rotation_system.h"
#include "object.h"
#include "graphics.h"
#include "vector.h"
#include "maths.h"

void
system_ref_t::paint (object_t const & object) const {
  real const (& A) [3] = g [object.locus_begin];
  real const (& B) [3] = g [object.locus_end];

  real alpha = object.generator_position;
  real one_minus_alpha = 1 - alpha;
  real abc [3], generator [3];

  abc [0] = one_minus_alpha * A [0] + alpha * B [0];
  abc [1] = one_minus_alpha * A [1] + alpha * B [1];
  abc [2] = one_minus_alpha * A [2] + alpha * B [2];

  generator [0] = abc [0] * u [0] [0] + abc [1] * v [0] [0] + abc [2] * w [0] [0];
  generator [1] = abc [0] * u [0] [1] + abc [1] * v [0] [1] + abc [2] * w [0] [1];
  generator [2] = abc [0] * u [0] [2] + abc [1] * v [0] [2] + abc [2] * w [0] [2];

  real adjust (std::pow (generator [0] * generator [0] +
                         generator [1] * generator [1] +
                         generator [2] * generator [2], real (- 0.5)));

  for (unsigned n = 0; n != N / p; ++ n) {
    real a = abc [0] * adjust;
    scalar_multiply (a, u [n], au [n]);
  }

  for (unsigned n = 0; n != N / q; ++ n) {
    real b = abc [1] * adjust;
    scalar_multiply (b, v [n], bv [n]);
  }

  for (unsigned n = 0; n != N / r; ++ n) {
    real c = abc [2] * adjust;
    scalar_multiply (c, w [n], cw [n]);
  }

  if (object.chirality) {
    paint_snub_triangle_pairs (object.chirality, N / p, x, s, au, bv, cw);
    paint_snub_pgons (object.chirality, N / q, q, y, v, bv, cw, au);
    paint_snub_pgons (object.chirality, N / r, r, z, w, cw, au, bv);
  }
  else {
    paint_kpgons (2, N / p, p, x, u, au, bv, cw);
    paint_kpgons (2, N / q, q, y, v, bv, cw, au);
    paint_kpgons (2, N / r, r, z, w, cw, au, bv);
  }
}

void system_ref_t::save_display_lists (int list) {
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

    real const (& abc) [3] = g [generator];

    for (unsigned n = 0; n != N / p; ++ n) scalar_multiply (abc [0], u [n], au [n]);
    for (unsigned n = 0; n != N / q; ++ n) scalar_multiply (abc [1], v [n], bv [n]);
    for (unsigned n = 0; n != N / r; ++ n) scalar_multiply (abc [2], w [n], cw [n]);

    begin_list (lists_start + i);

    if (chirality) {
      paint_snub_triangle_pairs (chirality, N / p, x, s, au, bv, cw);
      paint_snub_pgons (chirality, N / q, q, y, v, bv, cw, au);
      paint_snub_pgons (chirality, N / r, r, z, w, cw, au, bv);
    }
    else {
      const unsigned pk [7] = { 0, 0, 0, 2, 0, 0, 2, };
      const unsigned qk [7] = { 1, 0, 1, 1, 2, 1, 2, };
      const unsigned rk [7] = { 1, 1, 0, 1, 1, 2, 2, };
      paint_kpgons (pk [generator], N / p, p, x, u, au, bv, cw);
      paint_kpgons (qk [generator], N / q, q, y, v, bv, cw, au);
      paint_kpgons (rk [generator], N / r, r, z, w, cw, au, bv);
    }
    end_list ();
  }
}

system_ref_t const * system_repository_t::ref (system_select_t select) const {
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
void initialize_system (system_ref_t & system_ref, const system_t <q, r> & system, real (* scratch) [3]) {
  enum { p = 2 };
  system_ref.p = p;
  system_ref.q = q;
  system_ref.r = r;
  system_ref.N = system.N;
  system_ref.x = system.x;
  system_ref.y = system.y;
  system_ref.z = system.z;
  system_ref.s = system.s;
  system_ref.u = system.u;
  system_ref.v = system.v;
  system_ref.w = system.w;
  system_ref.g = system.g;
  system_ref.au = scratch;
  system_ref.bv = scratch + system.N / p;
  system_ref.cw = scratch + system.N / p + system.N / q;
}

void system_repository_t::initialize (void * data) {
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
