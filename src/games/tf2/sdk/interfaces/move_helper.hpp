/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/interfaces/move_helper.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef MOVE_HELPER_HPP
#define MOVE_HELPER_HPP

class Player;

class MoveHelper {
public:
  void set_host(Player* host) {
    if (this == nullptr) {
      return;
    }

    void** vtable = *(void***)this;
    if (vtable == nullptr) {
      return;
    }

    auto set_host_fn = reinterpret_cast<void (*)(void*, Player*)>(vtable[12]);
    if (set_host_fn == nullptr) {
      return;
    }

    set_host_fn(this, host);
  }
};

inline static MoveHelper* move_helper;

#endif
