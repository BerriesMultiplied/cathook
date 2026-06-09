/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/scene_end.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/automation/nographics/nographics.hpp"

void (*scene_end_original)(void*, void*);

void scene_end_hook(void* me, void* b8) {
  if (nographics::should_skip_rendering_hooks()) {
    scene_end_original(me, b8);
    return;
  }

  scene_end_original(me, b8);
  //chams();
}
