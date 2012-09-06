#include "object.h"
#include "system_ref.h"
#include "markov.h"
#include "random.h"
#include "config.h"

void object_t::start (rng_t & rng)
{
  locus_begin = rng.get () % 8;
  locus_end = markov (rng, locus_begin, locus_begin);
  generator_position = 0;
  display_list = 0;
  chirality = ((locus_end == 7 || locus_begin == 7) ? (1 - (rng.get () & 2)) : 0);
}

void object_t::update_appearance (float animation_time, rng_t & rng)
{
  intensity = usr::intensity_bump (animation_time);
  saturation = usr::saturation_bump (animation_time);

  // Adjust `locus_begin', `locus_end', `generator_position' and
  // `chirality', the parameters used in the function `paint_polyhedron'
  // in "graphics.cpp" to locate a point on the Moebius triangle.
  // The vertices of the polyhedron to be drawn are obtained by
  // plotting that point upon every tile of the spherical triangulation.

  static const float T0 = usr::morph_start;
  static const float T1 = usr::morph_finish;
  float t = animation_time;

  if (t < T0) {
    if (generator_position) {
      display_list = 0;
      // We must perform a Markov transition.
      unsigned next = markov (rng, locus_end, locus_begin);
      locus_begin = locus_end;
      locus_end = next;
      // Don't modify `chirality' if we are about to desnubify.
      if (locus_begin != 7) {
        chirality = locus_end == 7 ? (1 - (rng.get () & 2)) : 0;
      }
    }
    if (display_list == 0) {
      generator_position = 0;
      display_list = system_ref->lists_start + locus_begin;
      if (locus_begin == 7 && chirality == -1) {
        ++ display_list;
      }
    }
  }
  else if (t < T1) {
    float s = (t - T0) / (T1 - T0);
    generator_position = s * s * (3 - 2 * s);
    display_list = 0;
  }
  else {
    if (display_list == 0) {
      generator_position = 1;
      display_list = system_ref->lists_start + locus_end;
      if (locus_end == 7 && chirality == -1) {
        ++ display_list;
      }
    }
  }
}
