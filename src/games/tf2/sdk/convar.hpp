/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/convar.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef CONVAR_HPP
#define CONVAR_HPP

class Convar {
public:
  void set_int(int value) {
    *(int*)(this + 0x58) = value;
  }

  int get_int() {
    return *(int*)(this + 0x58);
  }

  void set_float(float value) {
    *(float*)(this + 0x54) = value;
  }

  float get_float() {
    return *(float*)(this + 0x54);
  }

};

#endif
