/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/draw_model_execute.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/visuals/chams/chams.hpp"

#include <string.h>

#include "core/types.hpp"
#include "core/assert.hpp"

#include "features/automation/nographics/nographics.hpp"
#include "features/menu/config.hpp"
#include "features/visuals/glow/player_model_glow.hpp"

#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/material_system.hpp"
#include "games/tf2/sdk/interfaces/model_render.hpp"
#include "games/tf2/sdk/interfaces/render_view.hpp"

#include "games/tf2/sdk/materials/material.hpp"
#include "games/tf2/sdk/entities/player.hpp"

#include "features/visuals/chams/chams.cpp"

#include "core/print.hpp"

struct ModelRenderInfo {
  Vec3 origin;
  Vec3 angles;
  void* pRenderable;
  const model_t* model;
  const VMatrix* model_to_world;
  const VMatrix* lighting_offset;
  const Vec3* lighting_origin;
  int flags;
  int entity_index;
  int skin;
  int body;
  int hitboxset;
  short instance;
};

void draw_model_execute_hook(void* me, void* state, ModelRenderInfo* pinfo, VMatrix* bone_to_world) {  
  if (nographics::should_skip_rendering_hooks()) {
    DME_RETURN;
  }

  if (pinfo == nullptr) {
    DME_RETURN;
  }

  if (player_model_glow::is_rendering()) {
    DME_RETURN;
  }
  
  Entity* entity = entity_list->entity_from_index(pinfo->entity_index);
  if (entity == nullptr) {
    DME_RETURN;
  }

  player_model_glow::note_rendered_model(entity);
  chams(entity, me, state, pinfo, bone_to_world);
  
  /*
  
  Material* material = materials.at(0);

  RGBA_float color = {1, 0, 1, 1};
  render_view->set_color_modulation(&color);
  render_view->set_blend(0.5);

  
  model_render->forced_material_override(material);
  material->set_material_flag(MATERIAL_VAR_WIREFRAME, true);

  draw_model_execute_original(me, state, pinfo, bone_to_world);

  model_render->forced_material_override(nullptr);

  RGBA_float white = {1,1,1,1};
  render_view->set_color_modulation(&white);
  render_view->set_blend(1);
  */
}
