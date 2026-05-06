/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/scene_end.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

void (*scene_end_original)(void*, void*);

void scene_end_hook(void* me, void* b8) {
  scene_end_original(me, b8);
  //chams();
}
