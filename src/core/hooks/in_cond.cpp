/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/in_cond.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/menu/config.hpp"
#include "core/detach.hpp"

bool in_cond_hook(void* me, int mask) {
  if (cathook::core::is_detach_pending()) {
    return false;
  }

  if (mask == TF_COND_ZOOMED && config.visuals.removals.scope == true) { //if the player is scoped
    return false;
  }

  auto* const original = in_cond_original;
  if (original == nullptr) {
    return false;
  }

  bool re = original(me, mask);
  
  return re;
}
