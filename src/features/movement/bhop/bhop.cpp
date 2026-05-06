/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/movement/bhop/bhop.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/menu/config.hpp"

#include "core/math/math.hpp"

#include "games/tf2/sdk/interfaces/client.hpp"
#include "games/tf2/sdk/interfaces/convar_system.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"

#include "games/tf2/sdk/entities/player.hpp"

#include <cmath>

namespace
{

[[nodiscard]] float normalize_2d_yaw(float yaw)
{
  return azimuth_to_signed(yaw);
}

[[nodiscard]] float vector_yaw(const Vec3& vector)
{
  return normalize_2d_yaw(std::atan2(vector.y, vector.x) * radpi);
}

[[nodiscard]] float vector_length_2d(const Vec3& vector)
{
  return std::sqrt((vector.x * vector.x) + (vector.y * vector.y));
}

void auto_jump(user_cmd* user_cmd, Player* localplayer)
{
  if (!config.misc.movement.bhop || user_cmd == nullptr || localplayer == nullptr) {
    return;
  }

  static bool static_jump = false;
  static bool static_grounded = false;
  static bool last_attempted = false;

  const bool last_jump = static_jump;
  const bool last_grounded = static_grounded;

  const bool current_jump = static_jump = (user_cmd->buttons & IN_JUMP) != 0;
  const bool current_grounded = static_grounded = localplayer->is_on_ground();

  if (current_jump && last_jump && (current_grounded ? !localplayer->is_ducking() : true)) {
    if (!(current_grounded && !last_grounded)) {
      user_cmd->buttons &= ~IN_JUMP;
    }

    if ((user_cmd->buttons & IN_JUMP) == 0 && current_grounded && !last_attempted) {
      user_cmd->buttons |= IN_JUMP;
    }
  }

  last_attempted = (user_cmd->buttons & IN_JUMP) != 0;
}

void auto_strafe_legit(user_cmd* user_cmd)
{
  if (user_cmd == nullptr || user_cmd->mouse_dx == 0) {
    return;
  }

  static Convar* cl_sidespeed = nullptr;
  if (cl_sidespeed == nullptr && convar_system != nullptr) {
    cl_sidespeed = convar_system->find_var("cl_sidespeed");
  }

  const float side_speed = cl_sidespeed != nullptr ? cl_sidespeed->get_float() : 450.0f;

  user_cmd->forwardmove = 0.0f;
  user_cmd->sidemove = user_cmd->mouse_dx > 0 ? side_speed : -side_speed;
}

void auto_strafe_directional(user_cmd* user_cmd, Player* localplayer)
{
  if (user_cmd == nullptr || localplayer == nullptr) {
    return;
  }

  if ((user_cmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)) == 0) {
    return;
  }

  const float forward_move = user_cmd->forwardmove;
  const float side_move = user_cmd->sidemove;

  Vec3 forward{};
  Vec3 right{};
  angle_vectors(user_cmd->view_angles, &forward, &right, nullptr);
  forward.z = 0.0f;
  right.z = 0.0f;

  const float forward_length = vector_length_2d(forward);
  const float right_length = vector_length_2d(right);
  if (forward_length <= 0.001f || right_length <= 0.001f) {
    return;
  }

  forward.x /= forward_length;
  forward.y /= forward_length;
  right.x /= right_length;
  right.y /= right_length;

  Vec3 wish_direction{
    (forward.x * forward_move) + (right.x * side_move),
    (forward.y * forward_move) + (right.y * side_move),
    0.0f
  };

  if (vector_length_2d(wish_direction) <= 0.001f) {
    return;
  }

  const Vec3 velocity = localplayer->get_velocity();
  if (vector_length_2d(velocity) <= 1.0f) {
    return;
  }

  const float direction_delta = normalize_2d_yaw(vector_yaw(wish_direction) - vector_yaw(velocity));
  if (std::fabs(direction_delta) > config.misc.movement.auto_strafe_max_delta) {
    return;
  }

  const float turn_scale = 0.9f + (0.1f * config.misc.movement.auto_strafe_turn_scale);
  const float rotation = ((direction_delta > 0.0f ? -90.0f : 90.0f) + (direction_delta * turn_scale)) * pideg;
  const float cos_rotation = std::cos(rotation);
  const float sin_rotation = std::sin(rotation);

  user_cmd->forwardmove = (cos_rotation * forward_move) - (sin_rotation * side_move);
  user_cmd->sidemove = (sin_rotation * forward_move) + (cos_rotation * side_move);
}

void auto_strafe(user_cmd* user_cmd, Player* localplayer)
{
  if (user_cmd == nullptr || localplayer == nullptr) {
    return;
  }

  if (config.misc.movement.auto_strafe == Misc::Movement::auto_strafe_mode::OFF) {
    return;
  }

  if (localplayer->is_on_ground() || (localplayer->get_flags() & FL_INWATER) != 0) {
    return;
  }

  switch (config.misc.movement.auto_strafe) {
  case Misc::Movement::auto_strafe_mode::LEGIT:
    auto_strafe_legit(user_cmd);
    break;
  case Misc::Movement::auto_strafe_mode::DIRECTIONAL:
    auto_strafe_directional(user_cmd, localplayer);
    break;
  case Misc::Movement::auto_strafe_mode::OFF:
  default:
    break;
  }
}

} // namespace

void bhop(user_cmd* user_cmd)
{
  if (user_cmd == nullptr || entity_list == nullptr) {
    return;
  }

  auto* localplayer = entity_list->get_localplayer();
  if (localplayer == nullptr || !localplayer->is_alive()) {
    return;
  }

  auto_jump(user_cmd, localplayer);
  auto_strafe(user_cmd, localplayer);
}
