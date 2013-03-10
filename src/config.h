// -*- C++ -*-

#ifndef config_h
#define config_h

#include "bump.h"

namespace usr {
  extern const unsigned fill_ratio;
  extern const float density;
  extern const float walls_friction;
  extern const float balls_friction;
  extern const float temperature;
  extern const float tank_distance;
  extern const float tank_depth;
  extern const float tank_height;
  extern const float morph_start;
  extern const float morph_finish;
  extern const float cycle_duration;
  extern const bump_specifier_t hsv_v_bump;
  extern const bump_specifier_t hsv_s_bump;
  extern const float frame_time;
  extern const float specular_material [4];
  extern const char * const window_name;
  extern const char * const message_window_name;
  extern const char * const window_class_name;
  extern const char * const message;
}
#endif

