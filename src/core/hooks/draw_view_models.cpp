/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/draw_view_models.cpp
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
#include "features/visuals/groups/visual_groups.hpp"
#include "features/visuals/thirdperson.hpp"

void (*draw_view_models_original)(void*, view_setup*, bool);

void draw_view_models_hook(void* me, view_setup* setup, bool draw_view_models) {
  if (nographics::should_skip_rendering_hooks()) {
    draw_view_models_original(me, setup, draw_view_models);
    return;
  }

  if (thirdperson::should_draw_local_player()) {
    draw_view_models = false;
  }

  if (draw_view_models) {
    visual_groups::begin_viewmodel_model();
  }

  draw_view_models_original(me, setup, draw_view_models);

  if (draw_view_models) {
    visual_groups::end_viewmodel_model();
  }

  player_model_glow::render_second();
}
