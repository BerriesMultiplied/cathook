/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/visuals/chams/chams.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "chams.hpp"

#include "core/assert.hpp"

#include "features/visuals/groups/visual_groups.hpp"

#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/material_system.hpp"

#include "games/tf2/sdk/materials/keyvalues.hpp"
#include "games/tf2/sdk/entities/player.hpp"

void chams(Entity* entity, void* me, void* state, ModelRenderInfo* pinfo, VMatrix* bone_to_world) {
  const visual_groups::visual_group_match group = visual_groups::group_for_entity(entity, true);
  if (!group) {
    DME_RETURN;
  }

  if (materials.empty()) {
    initialize_materials();
  }  

  // If we're still empty, something has gone very wrong.
  error_assert(materials.empty(), "Materials list is still empty even after initialization!");

  auto settings = get_chams_settings(*group);
  settings.color = visual_groups::color_for_entity(entity, *group);
  settings.color_z = settings.color;
  if (settings.material == nullptr && settings.material_z == nullptr) {
    draw_model_execute_original(me, state, pinfo, bone_to_world);
  } else {
    apply_chams_settings(me, state, pinfo, bone_to_world, settings);
  }

  model_render->forced_material_override(nullptr, OVERRIDE_NORMAL);
  RGBA_float white = {1,1,1,1};
  render_view->set_color_modulation(&white);
  render_view->set_blend(1);
}
