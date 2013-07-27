#include "config.h"

const unsigned usr::fill_ratio = 100;     // Number of balls per unit of screen aspect ratio.

// Physical parameters.
const float usr::min_radius = 1.0f;
const float usr::max_radius = 1.0f;
const float usr::density = 100.0f;        // Density of a ball.
const float usr::walls_friction = 0.075f;
const float usr::balls_friction = 0.075f;
const float usr::temperature = 212;       // Average kinetic energy.

// Container characteristics.
const float usr::tank_distance = 120.0f;  // Distancia del ojo a la pantalla.
const float usr::tank_depth = 22.0f;      // Tank depth in simulation units.
const float usr::tank_height = 22.0f;     // Height of screen / tank front.

// Parameters for lighting and animation timings.

//  r ^                        r = bump (t)
//    |
// v1 +--------------XXXXXX----------------
//    |           X  |    |  X
//    |         X    |    |    X
//    |        X     |    |     X
//    |      X       |    |       X
// v0 +XXXX----------+----+----------XXXX-->
//        t0        t1    t2        t3      t

//                                         v0     v1     t0     t1     t2     t3
const bump_specifier_t usr::hsv_v_bump = { 0.25f, 1.00f, 1.50f, 1.75f, 3.75f, 4.25f, };
const bump_specifier_t usr::hsv_s_bump = { 0.00f, 0.25f, 1.50f, 1.75f, 3.75f, 4.25f, };

const float usr::morph_start = 1.75f;
const float usr::morph_finish = 3.50f;
const float usr::cycle_duration = 4.25f;

const float usr::specular_material [4] = { 0.3f, 0.3f, 0.3f, 0.0f, };

// Simulation speed.
const float usr::frame_time = 1.0f / 60;

// Program name.
const char * const usr::window_name = "Polymorph";
const char * const usr::message_window_name = "Polymorph";
const char * const usr::window_class_name = "Polymorph";
const char * const usr::message =
 "And the ratios of their numbers, motions, and "
 "other properties, everywhere God, as far as "
 "necessity allowed or gave consent, has exactly "
 "perfected, and harmonised in due proportion.";
// "\n\nThis screensaver includes TinyScheme, developed by Dimitrios "
// "Souflis and licensed under the Modified BSD License. "
// "See \"tinyscheme/COPYING.txt\" for details.";
