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
  aimbot_reject_debug last_reject{};
  aimbot_reject_debug best_reject{};
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

inline aimbot_reject_reason reject_reason_for_skip(aimbot_player_skip_reason reason) {
  switch (reason) {
  case aimbot_player_skip_reason::invalid: return aimbot_reject_reason::invalid;
  case aimbot_player_skip_reason::local: return aimbot_reject_reason::local;
  case aimbot_player_skip_reason::dormant: return aimbot_reject_reason::dormant;
  case aimbot_player_skip_reason::dead: return aimbot_reject_reason::dead;
  case aimbot_player_skip_reason::invulnerable: return aimbot_reject_reason::invulnerable;
  case aimbot_player_skip_reason::ignored: return aimbot_reject_reason::ignored;
  case aimbot_player_skip_reason::friend_state: return aimbot_reject_reason::friend_state;
  case aimbot_player_skip_reason::ipc_bot: return aimbot_reject_reason::ipc_bot;
  case aimbot_player_skip_reason::cloaked: return aimbot_reject_reason::cloaked;
  case aimbot_player_skip_reason::team: return aimbot_reject_reason::team;
  case aimbot_player_skip_reason::type: return aimbot_reject_reason::type;
  case aimbot_player_skip_reason::none: break;
  }

  return aimbot_reject_reason::none;
}

inline bool reject_debug_better(const aimbot_reject_debug& candidate, const aimbot_reject_debug& best) {
  if (candidate.reason == aimbot_reject_reason::none) {
    return false;
  }
  if (best.reason == aimbot_reject_reason::none) {
    return true;
  }

  const bool candidate_has_fov = std::isfinite(candidate.fov) && candidate.fov < FLT_MAX;
  const bool best_has_fov = std::isfinite(best.fov) && best.fov < FLT_MAX;
  if (candidate_has_fov != best_has_fov) {
    return candidate_has_fov;
  }
  if (candidate_has_fov && candidate.fov != best.fov) {
    return candidate.fov < best.fov;
  }

  const bool candidate_has_distance = std::isfinite(candidate.distance) && candidate.distance < FLT_MAX;
  const bool best_has_distance = std::isfinite(best.distance) && best.distance < FLT_MAX;
  if (candidate_has_distance != best_has_distance) {
    return candidate_has_distance;
  }
  if (candidate_has_distance && candidate.distance != best.distance) {
    return candidate.distance < best.distance;
  }

  return candidate.entity_index > 0 && best.entity_index <= 0;
}

inline aimbot_reject_debug make_reject_debug(Entity* entity,
  aimbot_reject_reason reason,
  float fov = FLT_MAX,
  float fov_limit = FLT_MAX,
  float distance = FLT_MAX,
  int hitbox = -1,
  bool visible = false,
  bool preferred = false,
  bool current = false,
  bool backtrack = false,
  int trace_entity_index = -1,
  int trace_hitbox = -1) {
  aimbot_reject_debug debug{};
  debug.reason = reason;
  debug.fov = fov;
  debug.fov_limit = fov_limit;
  debug.distance = distance;
  debug.hitbox = hitbox;
  debug.visible = visible;
  debug.preferred = preferred;
  debug.current = current;
  debug.backtrack = backtrack;
  debug.trace_entity_index = trace_entity_index;
  debug.trace_hitbox = trace_hitbox;

  if (entity != nullptr) {
    debug.entity_index = entity->get_index();
    debug.team = static_cast<int>(entity->get_team());
    debug.health = aimbot_entity_health(entity);
  }

  return debug;
}

inline aimbot_reject_debug make_candidate_reject_debug(const aimbot_candidate& candidate,
  aimbot_reject_reason reason,
  float fov_limit = FLT_MAX,
  int trace_entity_index = -1,
  int trace_hitbox = -1) {
  return make_reject_debug(
    candidate.entity,
    reason,
    candidate.fov,
    fov_limit,
    candidate.distance,
    candidate.hitbox,
    candidate.visible,
    candidate.preferred,
    false,
    candidate.backtrack,
    trace_entity_index,
    trace_hitbox);
}

inline void record_reject(const aimbot_reject_debug& reject) {
  ++scan.candidates_rejected;
  scan.last_reject = reject;
  if (reject_debug_better(reject, scan.best_reject)) {
    scan.best_reject = reject;
  }
}

inline void record_player_skip(aimbot_player_skip_reason reason, Player* player = nullptr) {
  record_reject(make_reject_debug(player, reject_reason_for_skip(reason)));
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
