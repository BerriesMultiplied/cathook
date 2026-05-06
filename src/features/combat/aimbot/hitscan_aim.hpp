/*
/^-----^\   data: 2026-04-05
V  o o  V  file: src/features/combat/aimbot/hitscan_aim.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef HITSCAN_AIM_HPP
#define HITSCAN_AIM_HPP

#include "aim_utils.hpp"

inline aimbot_candidate hitscan_aim_find_candidate(Player* localplayer, Weapon* weapon, Player* player, const Vec3& original_view_angles) {
  aimbot_candidate candidate{};
  if (localplayer == nullptr || weapon == nullptr || player == nullptr) return candidate;

  aimbot_point point = aimbot_find_best_point(localplayer, player, weapon, original_view_angles, config.aimbot.hitscan_hitboxes);
  if (!point.valid) {
    return candidate;
  }

  candidate.entity = player;
  candidate.player = player;
  candidate.preferred = has_aimbot_preference(player);
  candidate.bone = point.bone;
  candidate.hitbox = point.hitbox;
  candidate.aim_position = point.position;
  candidate.aim_angles = point.angles;
  candidate.fov = point.fov;
  candidate.distance = distance_3d(localplayer->get_origin(), player->get_origin());
  candidate.health = player->get_health();
  candidate.visible = true;
  return candidate;
}

inline aimbot_candidate hitscan_aim_find_occluded_candidate(Player* localplayer, Weapon* weapon, Player* player, const Vec3& original_view_angles) {
  aimbot_candidate candidate{};
  if (localplayer == nullptr || weapon == nullptr || player == nullptr) return candidate;

  const int fallback_bone = aimbot_default_bone(localplayer, player, weapon);
  const Vec3 fallback_position = player->get_bone_pos(fallback_bone);

  candidate.entity = player;
  candidate.player = player;
  candidate.preferred = has_aimbot_preference(player);
  candidate.bone = fallback_bone;
  candidate.aim_position = fallback_position;
  candidate.aim_angles = aimbot_calculate_angles_to_position(localplayer->get_shoot_pos(), fallback_position);
  candidate.fov = aimbot_calculate_fov(candidate.aim_angles, original_view_angles);
  candidate.distance = distance_3d(localplayer->get_origin(), player->get_origin());
  candidate.health = player->get_health();
  candidate.visible = aimbot_trace_visible_to_position(localplayer, player, fallback_position);
  return candidate;
}

#endif
