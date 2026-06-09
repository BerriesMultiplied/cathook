/*
/^-----^\   data: 2026-05-06
V  o o  V  file: src/core/hooks/do_post_screen_space_effects.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "core/types.hpp"

#include "features/automation/nographics/nographics.hpp"
#include "features/visuals/glow/player_model_glow.hpp"

bool (*do_post_screen_space_effects_original)(void*, view_setup*);

bool do_post_screen_space_effects_hook(void* me, view_setup* setup)
{
  if (nographics::should_skip_rendering_hooks()) {
    return do_post_screen_space_effects_original(me, setup);
  }

  player_model_glow::render_first();

  return do_post_screen_space_effects_original(me, setup);
}
