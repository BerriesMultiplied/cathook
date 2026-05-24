#ifndef AIM_WALK_HPP
#define AIM_WALK_HPP

#include <algorithm>
#include <cmath>

#include "aim_state.hpp"
#include "aim_utils.hpp"

namespace aim_walk {

inline void request(Player* localplayer, Weapon* weapon, const aimbot_candidate& candidate) {
  const float current_time = global_vars != nullptr ? global_vars->curtime : 0.0f;
  if (current_time >= aim_state::walk_locked_until) {
    aim_state::clear_walk();
  }

  if (!config.aimbot.melee_walk_to_target) {
    aim_state::clear_walk();
    return;
  }

  if (localplayer == nullptr || weapon == nullptr || !aimbot_is_melee_weapon(weapon) || candidate.player == nullptr) {
    return;
  }

  constexpr float walk_to_target_distance = 82.0f;
  const float distance = distance_3d(localplayer->get_origin(), candidate.player->get_origin());
  if (distance > walk_to_target_distance) {
    return;
  }

  aim_state::walk_to_target = true;
  aim_state::walk_target = candidate.player->get_origin();
  aim_state::walk_locked_until = current_time + 0.18f;
}

inline void apply(Player* localplayer, user_cmd* user_cmd) {
  if (!aim_state::walk_to_target || localplayer == nullptr || user_cmd == nullptr) {
    return;
  }

  if (!config.aimbot.melee_walk_to_target) {
    aim_state::clear_walk();
    return;
  }

  const float current_time = global_vars != nullptr ? global_vars->curtime : 0.0f;
  if (current_time >= aim_state::walk_locked_until) {
    aim_state::clear_walk();
    return;
  }

  if (current_time < aim_state::walk_blocked_until) {
    return;
  }

  const Vec3 local_origin = localplayer->get_origin();
  if (aim_state::walk_last_progress_time <= 0.0f || aimbot_distance_squared(local_origin, aim_state::walk_last_origin) > 4.0f) {
    aim_state::walk_last_origin = local_origin;
    aim_state::walk_last_progress_time = current_time;
  } else if (current_time - aim_state::walk_last_progress_time > 0.35f) {
    aim_state::walk_blocked_until = current_time + 0.3f;
    user_cmd->buttons |= IN_JUMP;
    return;
  }

  const Vec3 delta{
    aim_state::walk_target.x - local_origin.x,
    aim_state::walk_target.y - local_origin.y,
    0.0f
  };
  const float planar_distance = std::sqrt((delta.x * delta.x) + (delta.y * delta.y));
  if (planar_distance <= 1.0f) {
    aim_state::clear_walk();
    return;
  }

  constexpr float walk_to_target_distance = 82.0f;
  constexpr float walk_to_target_release_distance = 96.0f;
  if (planar_distance > walk_to_target_release_distance) {
    aim_state::clear_walk();
    return;
  }

  constexpr float walk_to_target_speed = 450.0f;
  const float yaw = std::atan2(delta.y, delta.x) * radpi;
  const float yaw_delta = (yaw - user_cmd->view_angles.y) * pideg;
  const float speed_scale = std::clamp(planar_distance / walk_to_target_distance, 0.35f, 1.0f);
  const float move_speed = walk_to_target_speed * speed_scale;
  user_cmd->forwardmove = std::cos(yaw_delta) * move_speed;
  user_cmd->sidemove = -std::sin(yaw_delta) * move_speed;

  user_cmd->buttons &= ~(IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
  user_cmd->buttons |= IN_FORWARD;

  if (aim_state::walk_target.z - local_origin.z > 24.0f && planar_distance < 72.0f && localplayer->is_on_ground()) {
    user_cmd->buttons |= IN_JUMP;
  }
}

}

#endif
