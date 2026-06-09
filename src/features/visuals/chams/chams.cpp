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

#include "features/combat/anti_aim/anti_aim.hpp"
#include "features/visuals/groups/visual_groups.hpp"
#include "features/visuals/thirdperson.hpp"

#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/material_system.hpp"

#include "games/tf2/sdk/materials/keyvalues.hpp"
#include "games/tf2/sdk/entities/player.hpp"

namespace
{

bool rendering_fake_angle_chams = false;

[[nodiscard]] bool should_draw_fake_angle_chams(Entity* entity)
{
  if (rendering_fake_angle_chams
      || entity == nullptr
      || entity_list == nullptr
      || !anti_aim::has_visual_angles()
      || !thirdperson::should_draw_local_player()) {
    return false;
  }

  auto* localplayer = entity_list->get_localplayer();
  return localplayer != nullptr && entity == localplayer->to_entity();
}

[[nodiscard]] bool has_fake_angle_group(Entity* entity)
{
  visual_groups::begin_fake_angle_model();
  const bool result = static_cast<bool>(visual_groups::group_for_entity(entity, true));
  visual_groups::end_fake_angle_model();
  return result;
}

void draw_fake_angle_chams(Entity* entity)
{
  if (!should_draw_fake_angle_chams(entity) || !has_fake_angle_group(entity)) {
    return;
  }

  auto* localplayer = entity_list->get_localplayer();
  if (localplayer == nullptr) {
    return;
  }

  const Vec3 original_angles = localplayer->get_eye_angles();
  const Vec3 original_local_angles = localplayer->get_local_eye_angles();

  Vec3 fake_angles = anti_aim::fake_angles();
  fake_angles.z = 0.0f;

  rendering_fake_angle_chams = true;
  visual_groups::begin_fake_angle_model();
  localplayer->set_eye_angles(fake_angles);
  localplayer->set_local_eye_angles(fake_angles);
  entity->draw_model(STUDIO_RENDER);
  localplayer->set_eye_angles(original_angles);
  localplayer->set_local_eye_angles(original_local_angles);
  visual_groups::end_fake_angle_model();
  rendering_fake_angle_chams = false;
}

} // namespace

void chams(Entity* entity, void* me, void* state, ModelRenderInfo* pinfo, VMatrix* bone_to_world) {
  const visual_groups::visual_group_match group = visual_groups::group_for_entity(entity, true);
  if (!group) {
    draw_model_execute_original(me, state, pinfo, bone_to_world);
    draw_fake_angle_chams(entity);
    return;
  }

  if (materials.empty()) {
    initialize_materials();
  }  

  // If we're still empty, something has gone very wrong.
  error_assert(materials.empty(), "Materials list is still empty even after initialization!");

  auto settings = get_chams_settings(*group);
  settings.color = visual_groups::resolve_color(entity, *group, group->chams.visible_override_color, group->chams.visible_color);
  settings.color_z = visual_groups::resolve_color(entity, *group, group->chams.occluded_override_color, group->chams.occluded_color);
  if (settings.material == nullptr && settings.material_z == nullptr) {
    draw_model_execute_original(me, state, pinfo, bone_to_world);
  } else {
    apply_chams_settings(me, state, pinfo, bone_to_world, settings);
  }

  model_render->forced_material_override(nullptr, OVERRIDE_NORMAL);
  RGBA_float white = {1,1,1,1};
  render_view->set_color_modulation(&white);
  render_view->set_blend(1);
  draw_fake_angle_chams(entity);
}
