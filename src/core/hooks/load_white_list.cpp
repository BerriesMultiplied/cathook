/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/load_white_list.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/menu/config.hpp"

void* (*load_white_list_original)(void*, const char*);

void* load_white_list_hook(void* me, const char* str) {
  if (config.misc.exploits.bypasspure || config.misc.exploits.pure_bypass) {
    return nullptr;
  }

  return load_white_list_original(me, str);
}
