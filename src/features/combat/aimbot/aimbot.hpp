#ifndef AIMBOT_HPP
#define AIMBOT_HPP

#include "games/tf2/sdk/entities/player.hpp"
#include "games/tf2/sdk/interfaces/client.hpp"
#include "games/tf2/sdk/interfaces/global_vars.hpp"

namespace aimbot
{

struct aimbot_run_result {
  bool psilent_command = false;
  bool requested_shot = false;
  bool active_target = false;
};

struct aimbot_preference {
  Player* player = nullptr;
  float until_time = 0.0f;
};

struct aimbot_state {
  Player* target_player = nullptr;
  Entity* target_entity = nullptr;
  aimbot_preference preference{};
  Vec3 last_input_angles{};
  bool last_input_angles_valid = false;
  bool active_target = false;
  bool requested_shot = false;
  bool autoscope_cached_scoped = false;
  float autoscope_next_scope_check_time = 0.0f;
};

inline aimbot_state& current_state() {
  static aimbot_state value{};
  return value;
}

inline void clear_preference() {
  current_state().preference = {};
}

inline void set_preference(Player* player, float hold_for_seconds = 0.2f) {
  current_state().preference.player = player;
  current_state().preference.until_time = global_vars != nullptr
    ? global_vars->curtime + hold_for_seconds
    : hold_for_seconds;
}

inline bool has_preference(Player* player) {
  if (player == nullptr || current_state().preference.player != player) {
    return false;
  }

  return global_vars == nullptr || current_state().preference.until_time >= global_vars->curtime;
}

inline bool has_any_preference() {
  return has_preference(current_state().preference.player);
}

inline void clear_target_state() {
  current_state().target_player = nullptr;
  current_state().target_entity = nullptr;
  current_state().active_target = false;
  current_state().requested_shot = false;
  current_state().last_input_angles = {};
  current_state().last_input_angles_valid = false;
  current_state().autoscope_cached_scoped = false;
  current_state().autoscope_next_scope_check_time = 0.0f;
  clear_preference();
}

inline void clear_frame_target() {
  current_state().active_target = false;
  current_state().requested_shot = false;
}

inline void set_active_target(Entity* entity, Player* player) {
  current_state().target_entity = entity;
  current_state().target_player = player;
  current_state().active_target = entity != nullptr;
}

inline bool has_active_target() {
  return current_state().active_target;
}

inline Entity* active_target_entity() {
  return current_state().active_target ? current_state().target_entity : nullptr;
}

inline Player* active_target_player() {
  return current_state().active_target ? current_state().target_player : nullptr;
}

inline bool requested_shot() {
  return current_state().requested_shot;
}

inline void set_requested_shot(bool requested) {
  current_state().requested_shot = requested;
}

inline void store_input_angles(const Vec3& view_angles) {
  current_state().last_input_angles = view_angles;
  current_state().last_input_angles_valid = true;
}

inline void reset_input_history() {
  current_state().last_input_angles = {};
  current_state().last_input_angles_valid = false;
  current_state().active_target = false;
}

inline void reset_autoscope_state() {
  current_state().autoscope_cached_scoped = false;
  current_state().autoscope_next_scope_check_time = 0.0f;
}

inline bool autoscope_scoped_state(Player* localplayer) {
  if (localplayer == nullptr) {
    reset_autoscope_state();
    return false;
  }

  current_state().autoscope_cached_scoped = localplayer->is_scoped();
  current_state().autoscope_next_scope_check_time = global_vars != nullptr ? global_vars->curtime : 0.0f;
  return current_state().autoscope_cached_scoped;
}

aimbot_run_result run(user_cmd* cmd, const Vec3& original_view_angles);
void apply_walk_to_target(Player* localplayer, user_cmd* cmd);

}

#endif
