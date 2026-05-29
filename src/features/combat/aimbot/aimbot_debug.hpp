/*
/^-----^\   data: 2026-05-09
V  o o  V  file: src/features/combat/aimbot/aimbot_debug.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef AIMBOT_DEBUG_HPP
#define AIMBOT_DEBUG_HPP

#include <cfloat>

enum class aimbot_debug_reason {
  none,
  disabled,
  no_localplayer,
  no_weapon,
  no_target,
  use_key_inactive,
  auto_scope,
  auto_rev,
  attack_not_ready,
  scoped_only,
  headshot_wait,
  hitbox_miss,
  final_trace_miss,
  spread_seed_missing,
  attack_ready
};

enum class aimbot_reject_reason {
  none,
  invalid,
  local,
  dormant,
  dead,
  invulnerable,
  ignored,
  friend_state,
  ipc_bot,
  cloaked,
  team,
  type,
  no_candidate,
  not_visible,
  fov,
  projectile_hint_fov,
  projectile_no_solution,
  projectile_filter,
  no_model,
  no_studio_model,
  no_hitbox_set,
  no_hitbox,
  setup_bones,
  trace_blocked,
  wrong_hitbox,
  no_point
};

struct aimbot_reject_debug {
  aimbot_reject_reason reason = aimbot_reject_reason::none;
  int entity_index = -1;
  int team = -1;
  int health = 0;
  int hitbox = -1;
  int trace_entity_index = -1;
  int trace_hitbox = -1;
  float fov = FLT_MAX;
  float fov_limit = FLT_MAX;
  float distance = FLT_MAX;
  bool visible = false;
  bool preferred = false;
  bool current = false;
  bool backtrack = false;
};

struct aimbot_debug_state {
  bool active = false;
  bool requested_shot = false;
  bool attack_ready = false;
  bool scoped = false;
  bool scoped_ready = false;
  bool headshot_ready = false;
  bool final_trace_hit = false;
  bool spread_compensated = false;
  bool spread_signature = false;
  bool spread_fixed = false;
  int aim_mode = 0;
  int weapon_def_id = 0;
  int selected_entity_index = -1;
  int selected_hitbox = -1;
  int trace_entity_index = -1;
  int trace_hitbox = -1;
  int pellet_count = 0;
  int pellet_index = -1;
  int tick_count = 0;
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
  int resolver_mode = 0;
  int resolver_candidates = 0;
  int resolver_misses = 0;
  int resolver_hits = 0;
  float fov = FLT_MAX;
  float distance = FLT_MAX;
  float spread = 0.0f;
  float resolver_yaw = 0.0f;
  float resolver_pitch = 0.0f;
  bool resolver_active = false;
  aimbot_reject_debug last_reject{};
  aimbot_reject_debug best_reject{};
  aimbot_debug_reason reason = aimbot_debug_reason::none;
};

inline static aimbot_debug_state aimbot_debug_current_state{};

inline void aimbot_debug_set_state(const aimbot_debug_state& state) {
  aimbot_debug_current_state = state;
}

inline const aimbot_debug_state& aimbot_debug_get_state() {
  return aimbot_debug_current_state;
}

inline const char* aimbot_debug_reason_name(aimbot_debug_reason reason) {
  switch (reason) {
  case aimbot_debug_reason::none:
    return "none";
  case aimbot_debug_reason::disabled:
    return "disabled";
  case aimbot_debug_reason::no_localplayer:
    return "no local";
  case aimbot_debug_reason::no_weapon:
    return "no weapon";
  case aimbot_debug_reason::no_target:
    return "no target";
  case aimbot_debug_reason::use_key_inactive:
    return "key inactive";
  case aimbot_debug_reason::auto_scope:
    return "auto scope";
  case aimbot_debug_reason::auto_rev:
    return "auto rev";
  case aimbot_debug_reason::attack_not_ready:
    return "attack wait";
  case aimbot_debug_reason::scoped_only:
    return "scope wait";
  case aimbot_debug_reason::headshot_wait:
    return "head wait";
  case aimbot_debug_reason::hitbox_miss:
    return "body hit";
  case aimbot_debug_reason::final_trace_miss:
    return "trace miss";
  case aimbot_debug_reason::spread_seed_missing:
    return "spread seed";
  case aimbot_debug_reason::attack_ready:
    return "ready";
  }

  return "unknown";
}

inline const char* aimbot_debug_resolver_mode_name(int mode) {
  switch (mode) {
  case 1:
    return "moving";
  case 2:
    return "standing";
  case 3:
    return "jitter";
  case 4:
    return "spin";
  case 5:
    return "fakewalk";
  default:
    break;
  }

  return "unknown";
}

inline const char* aimbot_debug_reject_reason_name(aimbot_reject_reason reason) {
  switch (reason) {
  case aimbot_reject_reason::none:
    return "none";
  case aimbot_reject_reason::invalid:
    return "invalid";
  case aimbot_reject_reason::local:
    return "local";
  case aimbot_reject_reason::dormant:
    return "dormant";
  case aimbot_reject_reason::dead:
    return "dead";
  case aimbot_reject_reason::invulnerable:
    return "invuln";
  case aimbot_reject_reason::ignored:
    return "ignored";
  case aimbot_reject_reason::friend_state:
    return "friend";
  case aimbot_reject_reason::ipc_bot:
    return "ipc";
  case aimbot_reject_reason::cloaked:
    return "cloaked";
  case aimbot_reject_reason::team:
    return "team";
  case aimbot_reject_reason::type:
    return "type";
  case aimbot_reject_reason::no_candidate:
    return "no candidate";
  case aimbot_reject_reason::not_visible:
    return "not visible";
  case aimbot_reject_reason::fov:
    return "fov";
  case aimbot_reject_reason::projectile_hint_fov:
    return "hint fov";
  case aimbot_reject_reason::projectile_no_solution:
    return "no solution";
  case aimbot_reject_reason::projectile_filter:
    return "proj filter";
  case aimbot_reject_reason::no_model:
    return "no model";
  case aimbot_reject_reason::no_studio_model:
    return "no studio";
  case aimbot_reject_reason::no_hitbox_set:
    return "no hb set";
  case aimbot_reject_reason::no_hitbox:
    return "no hitbox";
  case aimbot_reject_reason::setup_bones:
    return "setup bones";
  case aimbot_reject_reason::trace_blocked:
    return "blocked";
  case aimbot_reject_reason::wrong_hitbox:
    return "wrong hb";
  case aimbot_reject_reason::no_point:
    return "no point";
  }

  return "unknown";
}

#endif
