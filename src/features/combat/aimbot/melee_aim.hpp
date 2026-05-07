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

inline bool melee_aim_is_knife(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  switch (weapon->get_def_id()) {
  case Spy_t_Knife:
  case Spy_t_KnifeR:
  case Spy_t_YourEternalReward:
  case Spy_t_ConniversKunai:
  case Spy_t_TheBigEarner:
  case Spy_t_TheWangaPrick:
  case Spy_t_TheSharpDresser:
  case Spy_t_TheSpycicle:
  case Spy_t_FestiveKnife:
  case Spy_t_TheBlackRose:
  case Spy_t_SilverBotkillerKnifeMkI:
  case Spy_t_GoldBotkillerKnifeMkI:
  case Spy_t_RustBotkillerKnifeMkI:
  case Spy_t_BloodBotkillerKnifeMkI:
  case Spy_t_CarbonadoBotkillerKnifeMkI:
  case Spy_t_DiamondBotkillerKnifeMkI:
  case Spy_t_SilverBotkillerKnifeMkII:
  case Spy_t_GoldBotkillerKnifeMkII:
    return true;
  default:
    return false;
  }
}

inline float melee_aim_impact_time(Player* localplayer, Weapon* weapon) {
  if (weapon == nullptr) {
    return 0.0f;
  }

  if (melee_aim_is_knife(weapon)) {
    return 0.0f;
  }

  const float current_time = global_vars != nullptr
    ? global_vars->curtime
    : (localplayer != nullptr ? localplayer->get_tickbase() * static_cast<float>(TICK_INTERVAL) : 0.0f);
  const float smack_time = weapon->get_smack_time();
  if (smack_time > current_time) {
    return std::clamp(smack_time - current_time, 0.0f, 0.35f);
  }

  return std::clamp(weapon->get_smack_delay(), 0.0f, 0.35f);
}

inline Vec3 melee_aim_player_center(Player* player, const Vec3& origin) {
  if (player == nullptr) {
    return origin;
  }

  const Vec3 mins = player->get_player_mins(player->is_ducking());
  const Vec3 maxs = player->get_player_maxs(player->is_ducking());
  return origin + Vec3{0.0f, 0.0f, mins.z + ((maxs.z - mins.z) * 0.5f)};
}

inline Vec3 melee_aim_local_swing_start(Player* localplayer, float impact_time) {
  if (localplayer == nullptr || impact_time <= 0.0001f) {
    return localplayer != nullptr ? localplayer->get_shoot_pos() : Vec3{};
  }

  LocalPredictionEntityPath local_path = local_prediction_predict_entity_path(localplayer, impact_time, false);
  if (!local_path.valid || local_path.positions.empty()) {
    return localplayer->get_shoot_pos();
  }

  return local_path.positions.back() + localplayer->get_view_offset();
}

inline bool melee_aim_segment_reaches_target(Player* target,
  const Vec3& target_origin,
  const Vec3& start,
  const Vec3& end,
  float melee_hull,
  float* enter_fraction_out = nullptr) {
  if (target == nullptr || melee_hull < 0.0f) {
    return false;
  }

  const Vec3 hull{melee_hull, melee_hull, melee_hull};
  const Vec3 target_mins = target->get_player_mins(target->is_ducking()) + target_origin - hull;
  const Vec3 target_maxs = target->get_player_maxs(target->is_ducking()) + target_origin + hull;
  Vec3 delta = end - start;
  float enter = 0.0f;
  float exit = 1.0f;

  const auto clip_axis = [&](float start_axis, float delta_axis, float min_axis, float max_axis) -> bool {
    if (std::fabs(delta_axis) <= 0.0001f) {
      return start_axis >= min_axis && start_axis <= max_axis;
    }

    const float inv_delta = 1.0f / delta_axis;
    float t1 = (min_axis - start_axis) * inv_delta;
    float t2 = (max_axis - start_axis) * inv_delta;
    if (t1 > t2) {
      std::swap(t1, t2);
    }

    enter = std::max(enter, t1);
    exit = std::min(exit, t2);
    return enter <= exit;
  };

  const bool intersects =
    clip_axis(start.x, delta.x, target_mins.x, target_maxs.x) &&
    clip_axis(start.y, delta.y, target_mins.y, target_maxs.y) &&
    clip_axis(start.z, delta.z, target_mins.z, target_maxs.z);
  if (intersects && enter_fraction_out != nullptr) {
    *enter_fraction_out = std::clamp(enter, 0.0f, 1.0f);
  }

  return intersects;
}

inline bool melee_aim_trace_candidate(Player* localplayer,
  Weapon* weapon,
  Player* target,
  const Vec3& target_origin,
  const Vec3& swing_start,
  const Vec3& aim_angles) {
  if (localplayer == nullptr || weapon == nullptr || target == nullptr || engine_trace == nullptr) {
    return false;
  }

  const float melee_range = aimbot_get_melee_range(localplayer, weapon, target);
  const float melee_hull = aimbot_get_melee_hull(localplayer, weapon, target);
  if (melee_range <= 0.0f || melee_hull < 0.0f) {
    return false;
  }

  Vec3 forward = local_prediction_angles_to_direction(aim_angles);
  if (!aimbot_vec3_is_finite(forward) || !aimbot_vec3_is_finite(swing_start)) {
    return false;
  }

  Vec3 end = swing_start + (forward * melee_range);
  float target_enter_fraction = 1.0f;
  if (!melee_aim_segment_reaches_target(target, target_origin, swing_start, end, melee_hull, &target_enter_fraction)) {
    return false;
  }

  Vec3 start = swing_start;
  trace_filter filter{};
  engine_trace->init_world_trace_filter(&filter);

  ray_t ray = engine_trace->init_ray(&start, &end);
  trace_t trace{};
  engine_trace->trace_ray(&ray, MASK_SOLID, &filter, &trace);
  if (trace.all_solid || trace.start_solid || trace.fraction + 0.001f < target_enter_fraction) {
    return false;
  }

  Vec3 hull_mins{-melee_hull, -melee_hull, -melee_hull};
  Vec3 hull_maxs{melee_hull, melee_hull, melee_hull};
  ray = engine_trace->init_ray(&start, &end, &hull_mins, &hull_maxs);
  trace = {};
  engine_trace->trace_ray(&ray, MASK_SOLID, &filter, &trace);
  return !trace.all_solid && !trace.start_solid && trace.fraction + 0.001f >= target_enter_fraction;
}

inline bool melee_aim_trace_candidate(Player* localplayer,
  Weapon* weapon,
  Player* target,
  const Vec3& target_origin,
  const Vec3& aim_angles) {
  if (localplayer == nullptr) {
    return false;
  }

  return melee_aim_trace_candidate(localplayer, weapon, target, target_origin, localplayer->get_shoot_pos(), aim_angles);
}

inline aimbot_candidate melee_aim_find_candidate(Player* localplayer,
  Weapon* weapon,
  Player* player,
  user_cmd*,
  const Vec3& original_view_angles) {
  aimbot_candidate candidate{};
  if (localplayer == nullptr || weapon == nullptr || player == nullptr) return candidate;

  const float impact_time = melee_aim_impact_time(localplayer, weapon);
  const float prediction_time = std::max(impact_time, static_cast<float>(TICK_INTERVAL));
  LocalPredictionEntityPath target_path = local_prediction_predict_entity_path(player, prediction_time, false, true);
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
  Vec3 swing_start = melee_aim_local_swing_start(localplayer, impact_time);
  Vec3 hitbox_offset = point.position - player->get_origin();
  Vec3 target_positions[] = {
    predicted_origin + hitbox_offset,
    melee_aim_player_center(player, predicted_origin),
    predicted_origin + Vec3{0.0f, 0.0f, std::clamp(swing_start.z - predicted_origin.z, player->get_player_mins(player->is_ducking()).z, player->get_player_maxs(player->is_ducking()).z)}
  };

  bool found_trace = false;
  Vec3 target_position{};
  Vec3 aim_angles{};
  float target_fov = FLT_MAX;
  float target_distance = FLT_MAX;
  for (const Vec3& trace_position : target_positions) {
    if (!aimbot_vec3_is_finite(trace_position)) {
      continue;
    }

    const Vec3 trace_angles = aimbot_calculate_angles_to_position(swing_start, trace_position);
    if (!melee_aim_trace_candidate(localplayer, weapon, player, predicted_origin, swing_start, trace_angles)) {
      continue;
    }

    const float trace_fov = aimbot_calculate_fov(trace_angles, original_view_angles);
    if (!found_trace || trace_fov < target_fov) {
      found_trace = true;
      target_position = trace_position;
      aim_angles = trace_angles;
      target_fov = trace_fov;
      target_distance = distance_3d(swing_start, trace_position);
    }
  }

  if (!found_trace) {
    return candidate;
  }

  candidate.entity = player;
  candidate.player = player;
  candidate.preferred = has_aimbot_preference(player);
  candidate.bone = point.bone;
  candidate.hitbox = point.hitbox;
  candidate.aim_position = target_position;
  candidate.aim_angles = aim_angles;
  candidate.fov = target_fov;
  candidate.distance = target_distance;
  candidate.health = player->get_health();
  candidate.visible = true;
  candidate.melee_has_prediction = true;
  candidate.melee_impact_time = target_path.start_time + impact_time;
  candidate.melee_swing_start = swing_start;
  candidate.melee_target_origin = predicted_origin;
  return candidate;
}

#endif
