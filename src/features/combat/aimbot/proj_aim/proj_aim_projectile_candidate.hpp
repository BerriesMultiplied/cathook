#ifndef PROJ_AIM_PROJECTILE_CANDIDATE_HPP
#define PROJ_AIM_PROJECTILE_CANDIDATE_HPP

#include <cmath>

#include "../aimbot.hpp"
#include "proj_aim_types.hpp"

inline aimbot_candidate proj_aim_build_projectile_candidate(Player* player,
  const proj_aim_projectile_candidate_input& input,
  const LocalPredictionInterceptResult& intercept) {
  aimbot_candidate candidate{};
  if (player == nullptr || input.point == nullptr || !intercept.valid) {
    return candidate;
  }

  candidate.entity = player;
  candidate.player = player;
  candidate.preferred = aimbot::has_preference(player);
  candidate.bone = input.point->bone;
  candidate.hitbox = input.point->hitbox;
  candidate.studio_hitbox = input.point->studio_hitbox;
  candidate.aim_position = input.splash ? input.aim_position : intercept.target_origin;
  candidate.aim_angles = intercept.aim_angles;
  candidate.fov = input.fov;
  candidate.distance = input.distance;
  candidate.health = player->get_health();
  candidate.visible = true;
  candidate.projectile_direct = !input.splash;
  candidate.projectile_splash = input.splash;
  candidate.projectile_has_target_base_origin = intercept.has_target_base_origin;
  candidate.projectile_intercept_time = intercept.intercept_time;
  candidate.projectile_miss_distance = input.miss_distance;
  candidate.projectile_splash_radius = input.splash_radius;
  candidate.projectile_target_base_origin = input.target_base_origin;
  candidate.projectile_target_offset = input.target_offset;
  candidate.projectile_target_velocity = input.target_velocity;
  return candidate;
}

inline bool proj_aim_projectile_candidate_better(const aimbot_candidate& candidate,
  int candidate_priority,
  const aimbot_candidate& best,
  int best_priority) {
  if (candidate.entity == nullptr) {
    return false;
  }
  if (best.entity == nullptr) {
    return true;
  }

  if (candidate_priority != best_priority) {
    return candidate_priority < best_priority;
  }

  constexpr float miss_epsilon = 1.0f;
  if (candidate.projectile_miss_distance + miss_epsilon < best.projectile_miss_distance) {
    return true;
  }
  if (best.projectile_miss_distance + miss_epsilon < candidate.projectile_miss_distance) {
    return false;
  }
  if (std::fabs(candidate.fov - best.fov) > 0.01f) {
    return candidate.fov < best.fov;
  }
  if (std::fabs(candidate.distance - best.distance) > 0.001f) {
    return candidate.distance < best.distance;
  }
  if (std::fabs(candidate.projectile_intercept_time - best.projectile_intercept_time) > 0.001f) {
    return candidate.projectile_intercept_time < best.projectile_intercept_time;
  }

  return false;
}

#endif
