// -*- C++ -*-

#ifndef config_h
#define config_h

#include "bump.h"

namespace usr {
  // Number of objects.
  extern const unsigned count;        // Number of balls.

  // Physical parameters.
  extern const float mass;            // Mass of a ball.
  extern const float walls_friction;
  extern const float balls_friction;
  extern const float temperature;     // Average kinetic energy.

  // Container characteristics.
  extern const float tank_distance;   // Distancia del ojo a la pantalla.
  extern const float tank_depth;      // Tank depth in simulation units.
  extern const float tank_height;     // Height of screen / tank front.

  // Parameters for lighting and animation timings.
  extern const float morph_start;
  extern const float morph_finish;
  extern const float cycle_duration;
  extern const bump_specifier_t hsv_v_bump;
  extern const bump_specifier_t hsv_s_bump;

  // Simulation speed.
  extern const float frame_time;

  // Lighting parameters.
  extern const float specular_material [4];

  // Program name.
  extern const char * const window_name;
  extern const char * const message_window_name;
  extern const char * const window_class_name;
  extern const char * const message;
}
#endif

