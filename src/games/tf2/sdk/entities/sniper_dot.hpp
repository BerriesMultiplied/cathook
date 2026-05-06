/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/games/tf2/sdk/entities/sniper_dot.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef SNIPER_DOT_HPP
#define SNIPER_DOT_HPP

class SniperDot {  
public:
  float get_charge_start_time(void) {
    return *(float*)(this + 0x7E0);
  }
};

#endif

