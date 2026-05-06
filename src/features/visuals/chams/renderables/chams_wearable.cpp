/*
/^-----^\   data: 2026-04-06
V  o o  V  file: src/features/visuals/chams/renderables/chams_wearable.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "../chams.hpp"

#include "games/tf2/sdk/entities/entity.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"

void chams_wearable(Entity* entity, void* me, void* state, ModelRenderInfo* pinfo, VMatrix* bone_to_world) {
  auto* localplayer = entity_list->get_localplayer();
  auto* owner_entity = entity->get_owner_entity();
  if (owner_entity == nullptr) DME_RETURN;
  if (owner_entity->get_class_id() != class_id::PLAYER) DME_RETURN;

  auto* owner_player = reinterpret_cast<Player*>(owner_entity);

  if (owner_player == localplayer && config.chams.player.local == false) DME_RETURN;
  if (entity->is_dormant()) DME_RETURN;
  if (entity->get_team() != localplayer->get_team() && config.chams.player.enemy == false && !owner_player->is_friend()) DME_RETURN;
  if (entity->get_team() == localplayer->get_team() && owner_player != localplayer && config.chams.player.team == false && !owner_player->is_friend()) DME_RETURN;
  if (owner_player->is_friend() && config.chams.player.friends == false && (config.chams.player.team == false && entity->get_team() == localplayer->get_team())) DME_RETURN;

  const auto settings = get_chams_settings(owner_player, localplayer);
  apply_chams_settings(me, state, pinfo, bone_to_world, settings);
}
