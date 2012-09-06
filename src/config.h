// -*- C++ -*-

#ifndef config_h
#define config_h

#include "real.h"
#include "bump.h"

namespace usr {
  // Number of objects.
  static const unsigned count = 400;          // Number of balls.

  // Physical properties.
  static const real mass = 35.0f;             // Mass of a ball.

  // Initial conditions.
  static const real temperature = 212;        // Average kinetic energy.

  // Container characteristics.
  static const real tank_distance = 90.0f;    // Distancia del ojo a la pantalla.
  static const real tank_depth = 60.0f;       // Tank depth in simulation units.
  static const real tank_height = 24.0f;      // Height of screen / tank front.
  static const real margin = 0.1f;

  static const real walls_friction = 0.010f;
  static const real balls_friction = 0.060f;

  // Animation timings.
  static const real morph_start = 0.50f;
  static const real morph_finish = 4.50f;
  static const real cycle_duration = 17.05f;

  // Colour functions.
  // { low_value, high_value, attack_begin, attack_end, decay_begin, decay_end }
  static const bump_t intensity_bump = { 0.10f, 1.00f, 0.35f, 0.50f, 4.50f, 6.00f, };
  static const bump_t saturation_bump = { 1.00f, 0.00f, 0.50f, 0.65f, 5.50f, 6.50f, };

  // Simulation speed.
  static const real simulation_rate = 1.0f;   // Rate of time.
  static const real max_frame_time = 0.55f;

  // Program name.
  static const char * const window_name = "Convex uniform polyhedra";
  static const char * const message_window_name = "Polymorph";
  static const char * const window_class_name = "Polymorph";
  static const char * const message =
  "And the ratios of their numbers, motions, and "
  "other properties, everywhere God, as far as "
  "necessity allowed or gave consent, has exactly "
  "perfected, and harmonised in due proportion.";
  // "\n\nThis screensaver includes TinyScheme, developed by Dimitrios "
  // "Souflis and licensed under the Modified BSD License. "
  // "See \"tinyscheme/COPYING.txt\" for details.";
}
#endif
