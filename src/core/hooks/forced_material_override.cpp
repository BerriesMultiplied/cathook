/*
/^-----^\   data: 2026-05-07
V  o o  V  file: src/core/hooks/forced_material_override.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/visuals/glow/player_model_glow.hpp"

#include "games/tf2/sdk/interfaces/model_render.hpp"
#include "games/tf2/sdk/materials/material.hpp"

void (*forced_material_override_original)(void*, Material*, OverrideType);

void forced_material_override_hook(void* me, Material* material, OverrideType override_type)
{
  if (player_model_glow::is_rendering()) {
    return;
  }

  forced_material_override_original(me, material, override_type);
}
