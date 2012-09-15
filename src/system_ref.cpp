#include "system_ref.h"
#include "nodes/system.h"
#include "object.h"
#include "graphics.h"
#include "vector.h"
#include "aligned-arrays.h"
#include "compiler.h"

void system_ref_t::paint (object_t const & object) const
{
  v4f alpha = _mm_set1_ps (object.generator_position);
  v4f one = { 1.0f, 1.0f, 1.0f, 1.0f, };
  v4f A = load4f (g [object.locus_begin]);
  v4f B = load4f (g [object.locus_end]);
  v4f abc = (one - alpha) * A + alpha * B;
  v4f a, b, c;
  UNPACK3 (abc, a, b, c);
  v4f t = a * load4f (x [0]) + b * load4f (y [0]) + c * load4f (z [0]);
  abc *= _mm_rsqrt_ps (dot (t, t));
  UNPACK3 (abc, a, b, c);

  for (unsigned n = 0; n != N / p; ++ n) store4f (ax [n], a * load4f (x [n]));
  for (unsigned n = 0; n != N / q; ++ n) store4f (by [n], b * load4f (y [n]));
  for (unsigned n = 0; n != N / r; ++ n) store4f (cz [n], c * load4f (z [n]));

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

    v4f abc = load4f (g [generator]), a, b, c;
    UNPACK3 (abc, a, b, c);
    for (unsigned n = 0; n != N / p; ++ n) store4f (ax [n], a * load4f (x [n]));
    for (unsigned n = 0; n != N / q; ++ n) store4f (by [n], b * load4f (y [n]));
    for (unsigned n = 0; n != N / r; ++ n) store4f (cz [n], c * load4f (z [n]));

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

namespace
{
  // Copy a two-dimensional array element-wise.
  NOINLINE // This gets very large if the loop is allowed to be unrolled.
  void assign (float (* & out) [4], const float (* & pvar) [4], const float (* from) [3], unsigned count)
  {
    for (unsigned n = 0; n != count; ++ n) {
      out [n] [0] = from [n] [0];
      out [n] [1] = from [n] [1];
      out [n] [2] = from [n] [2];
      out [n] [3] = 0.0f;
    }
    pvar = out;
    out += count;
  }
}

template <unsigned q, unsigned r>
void initialize_system (float (* & out) [4], system_ref_t & system_ref, const system_t <q, r> & system)
{
  enum { p = 2 };
  system_ref.p = p;
  system_ref.q = q;
  system_ref.r = r;
  system_ref.N = system.N;
  assign (out, system_ref.x, system.x, system.N / p);
  assign (out, system_ref.y, system.y, system.N / q);
  assign (out, system_ref.z, system.z, system.N / r);
  assign (out, system_ref.g, system.g, 8);
  system_ref.ax = out; out += system.N / p;
  system_ref.by = out; out += system.N / q;
  system_ref.cz = out; out += system.N / r;
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

  memory = allocate_internal (((62 + 26 + 14) * 2 + 3 * 8) * 4 * sizeof (float) + 32);
  auto out = (float (*) [4]) ((((std::intptr_t) (memory)) + 31) & -32);

  initialize_system (out, refs [0], t);
  initialize_system (out, refs [1], o);
  initialize_system (out, refs [2], i);

  lists_start = get_lists_start (27); // error handling?
  refs [0].save_display_lists (lists_start + 0 * 9);
  refs [1].save_display_lists (lists_start + 1 * 9);
  refs [2].save_display_lists (lists_start + 2 * 9);
}

system_repository_t:: ~system_repository_t ()
{
  deallocate (memory);
}
