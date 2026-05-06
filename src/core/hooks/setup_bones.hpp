/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/setup_bones.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef SETUP_BONES_HPP
#define SETUP_BONES_HPP

inline bool (*setup_bones_original)(void*, void*, int, int, float) = nullptr;

bool setup_bones_hook(void* me, void* bone_to_world_out, int max_bones, int bone_mask, float current_time);

#endif
