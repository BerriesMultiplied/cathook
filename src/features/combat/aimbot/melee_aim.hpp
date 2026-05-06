/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/combat/aimbot/melee_aim.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef MELEE_AIM_HPP
#define MELEE_AIM_HPP

#include "aim_utils.hpp"

inline bool melee_aim_trace_candidate(Player* localplayer,
  Weapon* weapon,
  Player* target,
  const Vec3& target_origin,
  const Vec3& aim_angles) {
  if (localplayer == nullptr || weapon == nullptr || target == nullptr) {
    return false;
  }

  const float melee_range = aimbot_get_melee_range(localplayer, weapon, target);
  const float melee_hull = aimbot_get_melee_hull(localplayer, weapon, target);
  if (melee_range <= 0.0f || melee_hull < 0.0f) {
    return false;
  }

  Vec3 start = localplayer->get_shoot_pos();
  Vec3 forward = local_prediction_angles_to_direction(aim_angles);
  Vec3 end = start + (forward * melee_range);

  Vec3 target_mins = target->get_player_mins(target->is_ducking()) + target_origin - Vec3{melee_hull, melee_hull, melee_hull};
  Vec3 target_maxs = target->get_player_maxs(target->is_ducking()) + target_origin + Vec3{melee_hull, melee_hull, melee_hull};

  return aimbot_segment_intersects_aabb(start, end, target_mins, target_maxs);
}

inline aimbot_candidate melee_aim_find_candidate(Player* localplayer, Weapon* weapon, Player* player, const Vec3& original_view_angles) {
  aimbot_candidate candidate{};
  if (localplayer == nullptr || weapon == nullptr || player == nullptr) return candidate;

  constexpr float melee_swing_delay = 0.12f;
  LocalPredictionEntityPath target_path = local_prediction_predict_entity_path(player, melee_swing_delay);
  if (!target_path.valid || target_path.positions.empty()) {
    return candidate;
  }

  aimbot_point point = aimbot_find_best_point(
    localplayer,
    player,
    weapon,
    original_view_angles,
    config.aimbot.melee_hitboxes,
    false);
  if (!point.valid) {
    return candidate;
  }

  Vec3 predicted_origin = target_path.positions.back();
  Vec3 hitbox_offset = point.position - player->get_origin();
  Vec3 target_position = predicted_origin + hitbox_offset;
  Vec3 aim_angles = aimbot_calculate_angles_to_position(localplayer->get_shoot_pos(), target_position);
  float distance = distance_3d(localplayer->get_shoot_pos(), target_position);
  float melee_range = aimbot_get_melee_range(localplayer, weapon, player);
  if (distance > melee_range + aimbot_get_melee_hull(localplayer, weapon, player)) {
    return candidate;
  }

  if (!melee_aim_trace_candidate(localplayer, weapon, player, predicted_origin, aim_angles)) {
    return candidate;
  }

  candidate.entity = player;
  candidate.player = player;
  candidate.preferred = has_aimbot_preference(player);
  candidate.bone = point.bone;
  candidate.hitbox = point.hitbox;
  candidate.aim_position = target_position;
  candidate.aim_angles = aim_angles;
  candidate.fov = aimbot_calculate_fov(candidate.aim_angles, original_view_angles);
  candidate.distance = distance;
  candidate.health = player->get_health();
  candidate.visible = true;
  return candidate;
}

#endif
