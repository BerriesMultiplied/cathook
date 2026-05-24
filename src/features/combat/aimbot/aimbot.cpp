#include "aimbot.hpp"

#include "aim_state.hpp"
#include "aim_spread.hpp"
#include "aim_auto_shoot.hpp"
#include "aim_walk.hpp"
#include "aim_targeting.hpp"
#include "aim_utils.hpp"
#include "aimbot_debug.hpp"
#include "hitscan_aim.hpp"
#include "melee_aim.hpp"
#include "proj_aim.hpp"
#include "resolver.hpp"

#include "core/entity_cache.hpp"
#include "features/combat/backtrack/backtrack.hpp"
#include "games/tf2/sdk/interfaces/engine.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/prediction.hpp"

namespace {

struct fire_readiness {
  bool projectile = false;
  bool hitscan = false;
  bool melee = false;
  bool headshot = false;
  bool charge = false;
  bool peek = false;
  bool projectile_visible_settled = false;
  bool tapfire_block = false;
  bool primary_allowed = false;

  bool ready() const {
    return projectile && hitscan && melee && headshot && charge && peek &&
      projectile_visible_settled && primary_allowed && !tapfire_block;
  }
};

struct aim_run {
  user_cmd* cmd;
  Player* local;
  Weapon* weapon;
  Vec3 source_angles;
  aimbot_debug_state debug{};

  aimbot_candidate best{};
  aimbot_candidate scope_target{};
  aim_spread::hitscan_fire_solution fire_solution{};
  aim_auto_shoot::result auto_shoot_result{};
  fire_readiness readiness{};

  Vec3 target_view_angles{};
  Vec3 visual_view_angles{};
  Vec3 projectile_view_angles{};

  bool projectile_solution = false;
  bool hitscan_solution = false;
  bool melee_solution = false;
  bool psilent_command = false;
  bool scoped_only_ready_value = false;

  bool finish(aimbot_debug_reason reason) {
    debug.reason = reason;
    debug.requested_shot = aim_state::requested_shot;
    aimbot_debug_set_state(debug);
    store_aimbot_input_angles(source_angles);
    return psilent_command;
  }
};

bool weapon_allows_primary_fire(Player* localplayer, Weapon* weapon) {
  return localplayer != nullptr && weapon != nullptr &&
    (localplayer->is_scoped() || weapon->get_def_id() != Sniper_m_TheMachina);
}

void apply_visible_view(user_cmd* cmd) {
  if (cmd == nullptr || config.aimbot.aim_mode == Aim::AimMode::PSILENT) {
    return;
  }

  Vec3 angles = cmd->view_angles;
  if (prediction != nullptr) {
    prediction->set_local_view_angles(angles);
    prediction->set_view_angles(angles);
  }
  if (engine != nullptr) {
    engine->set_view_angles(angles);
  }
}

bool peek_compensation_ready(Player* local, Weapon* weapon, const aimbot_candidate& candidate, bool hitscan_solution_ready) {
  if (!hitscan_solution_ready ||
      !aimbot_modifier_enabled(Aim::hitscan_mod_peek_compensation) ||
      candidate.entity == nullptr ||
      weapon == nullptr ||
      weapon->get_hitscan_spread() <= 0.00001f ||
      config.aimbot.peek_ticks <= 0) {
    return true;
  }

  const float tick_interval = global_vars != nullptr && global_vars->interval_per_tick > 0.0f
    ? global_vars->interval_per_tick
    : static_cast<float>(TICK_INTERVAL);
  Vec3 peek_origin = local->get_shoot_pos() -
    (local->get_velocity() * (tick_interval * static_cast<float>(config.aimbot.peek_ticks)));
  Vec3 end_pos = candidate.aim_position;

  if (!aimbot_trace_visible_to_position(local, candidate.entity, candidate.aim_position, aimbot_hitscan_trace_mask())) {
    return false;
  }

  ray_t ray = engine_trace->init_ray(&peek_origin, &end_pos);
  trace_filter filter{};
  engine_trace->init_trace_filter(&filter, local);
  trace_t trace{};
  engine_trace->trace_ray(&ray, aimbot_hitscan_trace_mask(), &filter, &trace);
  return trace.entity == candidate.entity ||
    (!trace.all_solid && !trace.start_solid && trace.fraction >= 0.999f);
}

bool reset_for_disabled(aim_run& run) {
  target_player = nullptr;
  target_entity = nullptr;
  clear_aimbot_preference();
  reset_autoscope_scope_state();
  reset_aimbot_scope_timing();
  reset_aimbot_input_history();
  return run.finish(aimbot_debug_reason::disabled);
}

bool validate_preconditions(aim_run& run) {
  if (!config.aimbot.master) {
    return reset_for_disabled(run);
  }

  run.local = entity_list->get_localplayer();
  if (run.local == nullptr || !run.local->is_alive()) {
    target_player = nullptr;
    target_entity = nullptr;
    reset_autoscope_scope_state();
    reset_aimbot_scope_timing();
    reset_aimbot_input_history();
    run.finish(aimbot_debug_reason::no_localplayer);
    return true;
  }

  update_aimbot_scope_timing(run.local);

  run.weapon = run.local->get_weapon();
  if (run.weapon == nullptr) {
    target_player = nullptr;
    target_entity = nullptr;
    reset_autoscope_scope_state();
    reset_aimbot_scope_timing();
    run.finish(aimbot_debug_reason::no_weapon);
    return true;
  }

  run.debug.weapon_def_id = run.weapon->get_def_id();
  run.projectile_solution = false;
  run.hitscan_solution = !aimbot_is_projectile_weapon(run.weapon) && !aimbot_is_melee_weapon(run.weapon);
  run.melee_solution = aimbot_is_melee_weapon(run.weapon);
  return false;
}

void cleanup_stale_targets(aim_run& run) {
  if (!entity_cache_snapshot_contains_player(target_player)) {
    target_player = nullptr;
  }
  if (aimbot_preference.preferred_target != nullptr &&
      !entity_cache_snapshot_contains_player(aimbot_preference.preferred_target)) {
    clear_aimbot_preference();
  }
  if (target_player != nullptr && aimbot_should_skip_player(run.local, target_player)) {
    target_player = nullptr;
  }
  if (aimbot_preference.preferred_target != nullptr &&
      aimbot_should_skip_player(run.local, aimbot_preference.preferred_target)) {
    clear_aimbot_preference();
  }
}

void populate_debug_state(aim_run& run) {
  auto& d = run.debug;
  d.candidates_total = aim_state::scan.candidates_total;
  d.candidates_visible = aim_state::scan.candidates_visible;
  d.candidates_rejected = aim_state::scan.candidates_rejected;
  d.skipped_ignored = aim_state::scan.skipped_ignored;
  d.skipped_friends = aim_state::scan.skipped_friends;
  d.skipped_ipc = aim_state::scan.skipped_ipc;
  d.skipped_cloaked = aim_state::scan.skipped_cloaked;
  d.skipped_team = aim_state::scan.skipped_team;
  d.skipped_invulnerable = aim_state::scan.skipped_invulnerable;
  d.skipped_dead = aim_state::scan.skipped_dead;
  d.skipped_type = aim_state::scan.skipped_type;

  if (run.best.entity == nullptr) {
    return;
  }
  d.selected_entity_index = run.best.entity->get_index();
  d.selected_hitbox = run.best.hitbox;
  d.fov = run.best.fov;
  d.distance = run.best.distance;
  d.tick_count = run.best.tick_count;
  if (run.best.player == nullptr) {
    return;
  }
  const auto info = resolver::debug_for_player(run.best.player);
  d.resolver_active = info.active;
  d.resolver_candidates = info.yaw_candidates;
  d.resolver_misses = info.misses;
  d.resolver_hits = info.hits;
  d.resolver_yaw = info.yaw;
  d.resolver_pitch = info.pitch;
  d.resolver_mode = static_cast<int>(info.mode);
}

void scan_targets(aim_run& run) {
  run.best = aim_targeting::find_best_candidate(run.local, run.weapon, run.cmd, run.source_angles);
  run.scope_target = run.best.player != nullptr
    ? run.best
    : aim_targeting::find_best_scope_candidate(run.local, run.weapon, run.source_angles);
  target_entity = run.best.entity;
  target_player = run.best.player;
  populate_debug_state(run);
}

void apply_pre_key_input(aim_run& run) {
  if (aimbot_should_auto_unscope(run.local, run.weapon, run.scope_target)) {
    run.cmd->buttons |= IN_ATTACK2;
  }
  if (aimbot_should_auto_unrev(run.local, run.weapon, run.best)) {
    run.cmd->buttons |= IN_ATTACK2;
  }
}

void compute_view_angles(aim_run& run) {
  run.target_view_angles = run.best.aim_angles - run.local->get_punch_angles();
  run.projectile_solution = run.best.projectile_direct || run.best.projectile_splash;
  run.visual_view_angles = aimbot_apply_mode_angles(
    run.source_angles,
    run.target_view_angles,
    aimbot_last_input_angles,
    aimbot_last_input_angles_valid,
    run.best);
  run.cmd->view_angles = run.visual_view_angles;

  const Vec3 projectile_base = run.projectile_solution ? run.target_view_angles : run.visual_view_angles;
  run.projectile_view_angles = run.projectile_solution
    ? aim_spread::apply_projectile_random_compensation(run.local, run.weapon, run.cmd, projectile_base)
    : run.visual_view_angles;
}

void compute_hitscan_fire_solution(aim_run& run) {
  if (!run.hitscan_solution) {
    return;
  }

  run.fire_solution = aim_spread::prepare_hitscan_fire_solution(
    run.local,
    run.weapon,
    run.cmd,
    run.best,
    run.best.player != nullptr ? run.best.command_angles : run.target_view_angles);
  if (run.fire_solution.ready) {
    run.best.command_angles = run.fire_solution.command_angles;
    run.best.spread_compensated = run.fire_solution.spread_compensated;
    run.best.pellet_index = run.fire_solution.pellet_index;
    run.best.pellet_count = run.fire_solution.pellet_count;
    run.best.spread = run.fire_solution.spread;
  }

  auto& d = run.debug;
  d.final_trace_hit = run.fire_solution.ready;
  d.spread_compensated = run.fire_solution.spread_compensated;
  d.spread_signature = run.fire_solution.spread_signature;
  d.spread_fixed = run.fire_solution.spread_fixed;
  d.spread = run.fire_solution.spread;
  d.pellet_count = run.fire_solution.pellet_count;
  d.pellet_index = run.fire_solution.pellet_index;
  d.trace_hitbox = run.fire_solution.trace_hitbox;
  d.trace_entity_index = run.fire_solution.trace_entity_index;
}

bool compute_melee_ready(const aim_run& run) {
  if (!run.melee_solution) {
    return true;
  }
  if (!aimbot_mode_uses_visible_steering() && run.best.visible) {
    return true;
  }
  if (run.best.player != nullptr) {
    return run.best.melee_has_prediction &&
      melee_aim_trace_candidate(
        run.local,
        run.weapon,
        run.best.player,
        run.best.melee_target_origin,
        run.best.melee_swing_start,
        run.cmd->view_angles);
  }
  return run.best.entity != nullptr &&
    aimbot_entity_melee_reachable(run.local, run.weapon, run.best.entity, run.cmd->view_angles);
}

bool compute_projectile_ready(const aim_run& run) {
  if (!run.projectile_solution) {
    return true;
  }
  if (!aimbot_mode_uses_visible_steering()) {
    return true;
  }
  if ((run.cmd->buttons & IN_ATTACK) != 0) {
    return true;
  }
  return aimbot_calculate_fov(run.projectile_view_angles, run.cmd->view_angles) <= aimbot_projectile_visible_settle_fov(run.best);
}

void compute_fire_readiness(aim_run& run) {
  auto& r = run.readiness;
  r.headshot = aimbot_wait_for_headshot_ready(run.local, run.weapon, run.best);
  r.charge = aimbot_wait_for_charge_ready(run.local, run.weapon, run.best);
  r.tapfire_block = aimbot_should_tapfire(run.local, run.weapon, run.best);
  if (!r.headshot || !r.charge || r.tapfire_block) {
    run.cmd->buttons &= ~IN_ATTACK;
  }

  r.projectile_visible_settled = compute_projectile_ready(run);
  r.projectile = !run.projectile_solution ||
    (r.projectile_visible_settled && run.best.visible) ||
    aim_targeting::projectile_solution_ready(run.local, run.weapon, run.cmd, run.best, run.projectile_view_angles);

  r.peek = peek_compensation_ready(run.local, run.weapon, run.best, run.hitscan_solution && run.fire_solution.ready);
  r.hitscan = (!run.hitscan_solution || run.fire_solution.ready) && r.peek;

  r.melee = compute_melee_ready(run);
  const bool secondary_blocks_attack =
    (run.cmd->buttons & IN_ATTACK2) != 0 &&
    !(run.projectile_solution && aim_auto_shoot::weapon_should_clear_secondary(run.weapon));
  r.primary_allowed = weapon_allows_primary_fire(run.local, run.weapon) && !secondary_blocks_attack;
  run.debug.headshot_ready = r.headshot;
  run.debug.attack_ready = r.ready();
}

void apply_post_attack_state(aim_run& run) {
  if (!run.readiness.ready() && (run.hitscan_solution || run.melee_solution)) {
    run.cmd->buttons &= ~IN_ATTACK;
  }
  if (run.projectile_solution && !run.readiness.ready()) {
    aim_auto_shoot::hold_charge_if_needed(run.cmd, run.weapon);
  }
  if (run.best.player != nullptr) {
    if (run.readiness.ready()) {
      set_aimbot_preference(run.best.player);
    } else if (aimbot_preference.preferred_target == run.best.player &&
        (run.hitscan_solution || !run.readiness.headshot || !run.scoped_only_ready_value)) {
      clear_aimbot_preference();
    }
  }
}

void apply_fire_command(aim_run& run) {
  if (config.aimbot.auto_shoot && run.readiness.ready()) {
    run.auto_shoot_result = aim_auto_shoot::apply(
      run.cmd,
      run.weapon,
      run.projectile_solution,
      run.hitscan_solution,
      run.melee_solution);
    aim_state::requested_shot = run.auto_shoot_result.requested;
  }

  const bool firing = (run.cmd->buttons & IN_ATTACK) != 0 || run.auto_shoot_result.release_attack;

  if (run.readiness.ready() && run.hitscan_solution && firing && run.fire_solution.ready) {
    run.cmd->view_angles = run.fire_solution.command_angles;
    if (run.best.player != nullptr) {
      resolver::note_shot(run.best.player, run.best.hitbox, run.best.simulation_time, run.best.backtrack);
    }
  }

  if (run.readiness.ready() && run.best.player != nullptr && firing && (run.hitscan_solution || run.melee_solution)) {
    run.cmd->tick_count = run.best.tick_count > 0
      ? run.best.tick_count
      : local_prediction_time_to_ticks(run.best.player->get_simulation_time() + backtrack::interpolation_time());
  }

  if (run.readiness.ready() && firing && run.projectile_solution) {
    run.cmd->view_angles = run.projectile_view_angles;
  }

  const bool psilent_mode = config.aimbot.aim_mode == Aim::AimMode::PSILENT;
  const bool primary = (run.cmd->buttons & IN_ATTACK) != 0;
  run.psilent_command = psilent_mode && run.readiness.ready() && (primary || run.auto_shoot_result.release_attack);
  if (psilent_mode && !run.psilent_command) {
    run.cmd->view_angles = run.source_angles;
  }

  apply_visible_view(run.cmd);
}

aimbot_debug_reason classify_outcome(const aim_run& run) {
  if (!run.readiness.headshot) {
    return aimbot_debug_reason::headshot_wait;
  }
  if (!run.readiness.hitscan) {
    if (run.fire_solution.seed_missing) return aimbot_debug_reason::spread_seed_missing;
    if (run.fire_solution.hit_wrong_hitbox) return aimbot_debug_reason::hitbox_miss;
    return aimbot_debug_reason::final_trace_miss;
  }
  if (!run.readiness.ready()) {
    return aimbot_debug_reason::attack_not_ready;
  }
  return aimbot_debug_reason::attack_ready;
}

}

bool aimbot(user_cmd* user_cmd, Vec3 original_view_angles) {
  aim_state::requested_shot = false;
  clear_aimbot_active_target();

  aim_run run{};
  run.cmd = user_cmd;
  run.source_angles = original_view_angles;
  run.debug.active = config.aimbot.master;
  run.debug.aim_mode = static_cast<int>(config.aimbot.aim_mode);

  if (validate_preconditions(run)) {
    return run.psilent_command;
  }
  cleanup_stale_targets(run);
  scan_targets(run);
  apply_pre_key_input(run);

  if (!aimbot_use_key_active()) {
    return run.finish(aimbot_debug_reason::use_key_inactive);
  }
  if (run.best.entity != nullptr) {
    set_aimbot_active_target();
  }
  if (aimbot_should_auto_scope(run.local, run.weapon, run.scope_target)) {
    run.cmd->buttons |= IN_ATTACK2;
    run.cmd->buttons &= ~IN_ATTACK;
    return run.finish(aimbot_debug_reason::auto_scope);
  }
  if (run.best.entity == nullptr) {
    aim_auto_shoot::hold_charge_if_needed(run.cmd, run.weapon);
    return run.finish(aimbot_debug_reason::no_target);
  }

  run.cmd->buttons &= ~IN_RELOAD;

  if (aimbot_should_auto_rev(run.local, run.weapon, run.best)) {
    run.cmd->buttons |= IN_ATTACK2;
    run.cmd->buttons &= ~IN_ATTACK;
    return run.finish(aimbot_debug_reason::auto_rev);
  }

  aim_walk::request(run.local, run.weapon, run.best);

  run.scoped_only_ready_value = aimbot_scoped_only_ready(run.local, run.weapon);
  run.debug.scoped = run.local->is_scoped();
  run.debug.scoped_ready = run.scoped_only_ready_value;
  if (!aim_auto_shoot::weapon_can_attack_or_release(run.local, run.weapon) || !run.scoped_only_ready_value) {
    if (!run.scoped_only_ready_value || (!aimbot_is_projectile_weapon(run.weapon) && !aimbot_is_melee_weapon(run.weapon))) {
      run.cmd->buttons &= ~IN_ATTACK;
    }
    if (!run.scoped_only_ready_value && run.best.player != nullptr && aimbot_preference.preferred_target == run.best.player) {
      clear_aimbot_preference();
    }
    aim_auto_shoot::hold_charge_if_needed(run.cmd, run.weapon);
    return run.finish(run.scoped_only_ready_value
      ? aimbot_debug_reason::attack_not_ready
      : aimbot_debug_reason::scoped_only);
  }

  compute_view_angles(run);
  compute_hitscan_fire_solution(run);
  compute_fire_readiness(run);
  apply_post_attack_state(run);
  apply_fire_command(run);

  return run.finish(classify_outcome(run));
}

bool aimbot_requested_shot() {
  return aim_state::requested_shot;
}

void aimbot_apply_walk_to_target(Player* localplayer, user_cmd* user_cmd) {
  aim_walk::apply(localplayer, user_cmd);
}
