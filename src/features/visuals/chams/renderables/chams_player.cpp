/*
/^-----^\   data: 2026-04-06
V  o o  V  file: src/features/visuals/chams/renderables/chams_player.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "../chams.hpp"

#include <algorithm>

#include "features/combat/backtrack/backtrack.hpp"

#include "games/tf2/sdk/interfaces/entity_list.hpp"

namespace
{

[[nodiscard]] chams_settings get_backtrack_chams_settings(float alpha_scale)
{
  auto settings = chams_settings{};
  settings.color = config.chams.player.backtrack_color;
  settings.color.a *= alpha_scale;
  settings.color_z = config.chams.player.backtrack_color_z;
  settings.color_z.a *= alpha_scale;
  settings.ignore_z = config.chams.player.backtrack_flags.ignore_z;
  settings.wireframe = is_wireframe_material(config.chams.player.backtrack_material_type);
  settings.wireframe_z = is_wireframe_material(config.chams.player.backtrack_material_z_type);
  settings.material = get_material(config.chams.player.backtrack_material_type);
  settings.material_z = get_material(config.chams.player.backtrack_material_z_type);
  return settings;
}

void draw_backtrack_chams(Player* player, void* me, void* state, ModelRenderInfo* pinfo)
{
  if (!config.chams.player.backtrack ||
      player == nullptr ||
      pinfo == nullptr) {
    return;
  }

  const auto* history = backtrack::records_for_player(player);
  if (history == nullptr || history->record_count <= 1) {
    return;
  }

  const int max_ticks = std::clamp(config.chams.player.backtrack_ticks, 1, backtrack::max_records);
  int drawn_records = 0;
  for (int record_index = history->record_count - 1; record_index >= 1 && drawn_records < max_ticks; --record_index) {
    const auto& record = history->records[record_index];
    if (!backtrack::is_record_valid(record, player) || record.bone_count <= 0) {
      continue;
    }

    const float alpha_scale = std::clamp(
      static_cast<float>(drawn_records + 1) / static_cast<float>(std::max(max_ticks, 1)),
      0.12f,
      1.0f);
    const auto settings = get_backtrack_chams_settings(alpha_scale);
    auto* bones = reinterpret_cast<VMatrix*>(const_cast<matrix_3x4*>(record.bones.data()));
    apply_chams_settings(me, state, pinfo, bones, settings, false);
    ++drawn_records;
  }
}

} // namespace

void chams_player(Player* player, void* me, void* state, ModelRenderInfo* pinfo, VMatrix* bone_to_world) {
  auto* localplayer = entity_list->get_localplayer();

  if (player == localplayer && config.chams.player.local == false) DME_RETURN;
  if (player->is_dormant()) DME_RETURN;
  if (!player->is_alive()) DME_RETURN;
  if (player->get_team() != localplayer->get_team() && config.chams.player.enemy == false && !player->is_friend()) {
    draw_backtrack_chams(player, me, state, pinfo);
    DME_RETURN;
  }
  if (player->get_team() == localplayer->get_team() && player != localplayer && config.chams.player.team == false && !player->is_friend()) DME_RETURN;
  if (player->is_friend() && config.chams.player.friends == false && (config.chams.player.team == false && player->get_team() == localplayer->get_team())) DME_RETURN;

  if (player->get_team() != localplayer->get_team() && !player->is_friend()) {
    draw_backtrack_chams(player, me, state, pinfo);
  }

  const auto settings = get_chams_settings(player, localplayer);
  apply_chams_settings(me, state, pinfo, bone_to_world, settings);
}
