/*
/^-----^\   data: 2026-04-06
V  o o  V  file: src/features/visuals/chams/renderables/chams_player.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "../chams.hpp"

#include "games/tf2/sdk/interfaces/entity_list.hpp"

void chams_player(Player* player, void* me, void* state, ModelRenderInfo* pinfo, VMatrix* bone_to_world) {
  auto* localplayer = entity_list->get_localplayer();

  if (player == localplayer && config.chams.player.local == false) DME_RETURN;
  if (player->is_dormant()) DME_RETURN;
  if (!player->is_alive()) DME_RETURN;
  if (player->get_team() != localplayer->get_team() && config.chams.player.enemy == false && !player->is_friend()) DME_RETURN;
  if (player->get_team() == localplayer->get_team() && player != localplayer && config.chams.player.team == false && !player->is_friend()) DME_RETURN;
  if (player->is_friend() && config.chams.player.friends == false && (config.chams.player.team == false && player->get_team() == localplayer->get_team())) DME_RETURN;

  const auto settings = get_chams_settings(player, localplayer);
  apply_chams_settings(me, state, pinfo, bone_to_world, settings);
}
