#ifndef AIM_STATE_HPP
#define AIM_STATE_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "aim_utils.hpp"

namespace aim_state {

struct scan_debug_stats {
  int candidates_total = 0;
  int candidates_visible = 0;
  int candidates_rejected = 0;
  int skipped_ignored = 0;
  int skipped_friends = 0;
  int skipped_ipc = 0;
  int skipped_cloaked = 0;
  int skipped_team = 0;
  int skipped_invulnerable = 0;
  int skipped_dead = 0;
  int skipped_type = 0;
};

struct auto_shoot_state {
  Weapon* weapon = nullptr;
  bool holding_charge = false;
  bool holding_overload = false;
};

inline bool requested_shot = false;
inline bool walk_to_target = false;
inline Vec3 walk_target{};
inline float walk_locked_until = 0.0f;
inline float walk_blocked_until = 0.0f;
inline Vec3 walk_last_origin{};
inline float walk_last_progress_time = 0.0f;
inline scan_debug_stats scan{};
inline auto_shoot_state auto_shoot{};

inline void clear_walk() {
  walk_to_target = false;
  walk_target = {};
  walk_last_origin = {};
  walk_last_progress_time = 0.0f;
}

inline void record_player_skip(aimbot_player_skip_reason reason) {
  ++scan.candidates_rejected;
  switch (reason) {
  case aimbot_player_skip_reason::ignored:      ++scan.skipped_ignored; break;
  case aimbot_player_skip_reason::friend_state: ++scan.skipped_friends; break;
  case aimbot_player_skip_reason::ipc_bot:      ++scan.skipped_ipc; break;
  case aimbot_player_skip_reason::cloaked:      ++scan.skipped_cloaked; break;
  case aimbot_player_skip_reason::team:         ++scan.skipped_team; break;
  case aimbot_player_skip_reason::invulnerable: ++scan.skipped_invulnerable; break;
  case aimbot_player_skip_reason::dead:         ++scan.skipped_dead; break;
  case aimbot_player_skip_reason::type:         ++scan.skipped_type; break;
  default: break;
  }
}

inline float actual_frame_time() {
  if (global_vars == nullptr) {
    return 0.0f;
  }

  float frame_time = 0.0f;
  if (std::isfinite(global_vars->frametime) && global_vars->frametime > 0.0f) {
    frame_time = std::max(frame_time, global_vars->frametime);
  }
  if (std::isfinite(global_vars->absolute_frametime) && global_vars->absolute_frametime > 0.0f) {
    frame_time = std::max(frame_time, global_vars->absolute_frametime);
  }
  return frame_time;
}

}

#endif
