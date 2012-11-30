// -*- C++ -*-

#ifndef config_h
#define config_h

namespace usr {
  // Number of objects.
  static const unsigned count = 400;          // Number of balls.

  // Physical properties.
  static const float mass = 80.0f;            // Mass of a ball.

  // Initial conditions.
  static const float temperature = 212;       // Average kinetic energy.

  // Container characteristics.
  static const float tank_distance = 90.0f;   // Distancia del ojo a la pantalla.
  static const float tank_depth = 60.0f;      // Tank depth in simulation units.
  static const float tank_height = 20.0f;     // Height of screen / tank front.

  static const float walls_friction = 0.075f;
  static const float balls_friction = 0.075f;

  // Animation timings.
  static const float morph_start = 0.50f;
  static const float morph_finish = 4.50f;
  static const float cycle_duration = 17.05f;

  // Simulation speed.
  static const float frame_time = 1.0f / 60;

  // Lighting parameters.
  static const float ambient_material [4] = { 0.01f, 0.01f, 0.01f, 1.0f, };
  static const float specular_material [4] = { 0.3f, 0.3f, 0.3f, 0.0f, };

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
