/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/movement/local_prediction/move_sim.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef MOVE_SIM_HPP
#define MOVE_SIM_HPP

#include <algorithm>
#include <array>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "MD5/MD5.hpp"

#include "core/random_seed.hpp"

#include "games/tf2/sdk/entities/player.hpp"
#include "games/tf2/sdk/entities/weapon.hpp"
#include "games/tf2/sdk/interfaces/attribute_manager.hpp"
#include "games/tf2/sdk/interfaces/client.hpp"
#include "games/tf2/sdk/interfaces/client_state.hpp"
#include "games/tf2/sdk/interfaces/convar_system.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/engine_trace.hpp"
#include "games/tf2/sdk/interfaces/game_movement.hpp"
#include "games/tf2/sdk/interfaces/global_vars.hpp"
#include "games/tf2/sdk/interfaces/move_helper.hpp"
#include "games/tf2/sdk/interfaces/prediction.hpp"

#include "core/math/math.hpp"
#include "features/menu/config.hpp"

enum class LocalPredictionRunMode {
  MOVEMENT_ONLY,
  FULL_COMMAND
};

struct LocalPredictionTick {
  user_cmd cmd{};
};

struct LocalPredictionTickResult {
  user_cmd cmd{};
  Vec3 origin{};
  Vec3 velocity{};
  Vec3 wish_velocity{};
  Vec3 jump_velocity{};
  float step_height = 0.0f;
  bool grounded = false;
};

struct LocalPredictionRequest {
  std::vector<LocalPredictionTick> ticks{};
  LocalPredictionRunMode run_mode = LocalPredictionRunMode::MOVEMENT_ONLY;
};

struct LocalPredictionResult {
  bool success = false;
  bool restored = false;
  int simulated_ticks = 0;
  std::vector<LocalPredictionTickResult> ticks{};
};

struct LocalPredictionDebugState {
  bool last_run_ok = false;
  bool last_restore_ok = false;
  int last_simulated_ticks = 0;
  int last_failure_stage = 0;
};

inline static LocalPredictionDebugState local_prediction_debug;

struct LocalPredictionPlayerSnapshot {
  Vec3 origin{};
  Vec3 abs_origin{};
  Vec3 velocity{};
  Vec3 base_velocity{};
  Vec3 view_offset{};
  int flags = 0;
  int ground_entity_handle = 0;
  int tickbase = 0;
  user_cmd* current_cmd = nullptr;
  bool ducked = false;
  bool ducking = false;
  bool in_duck_jump = false;
  float duck_time = 0.0f;
  float duck_jump_time = 0.0f;
  float fall_velocity = 0.0f;
};

struct LocalPredictionGlobalSnapshot {
  float curtime = 0.0f;
  float frametime = 0.0f;
  int tickcount = 0;
  bool first_time_predicted = false;
  bool in_prediction = false;
  int random_seed_value = -1;
};

struct LocalPredictionSnapshot {
  LocalPredictionPlayerSnapshot player{};
  LocalPredictionGlobalSnapshot globals{};
};

inline bool local_prediction_capture(Player* localplayer, LocalPredictionSnapshot* snapshot);
inline bool local_prediction_restore(Player* localplayer, const LocalPredictionSnapshot& snapshot);

struct local_prediction_bool_guard {
  bool* value = nullptr;
  bool previous = false;
  bool active = false;

  explicit local_prediction_bool_guard(bool* value_in, bool next_value)
    : value(value_in) {
    if (value == nullptr) {
      return;
    }

    previous = *value;
    *value = next_value;
    active = true;
  }

  ~local_prediction_bool_guard() {
    if (active && value != nullptr) {
      *value = previous;
    }
  }

  local_prediction_bool_guard(const local_prediction_bool_guard&) = delete;
  local_prediction_bool_guard& operator=(const local_prediction_bool_guard&) = delete;
};

struct local_prediction_restore_guard {
  Player* player = nullptr;
  LocalPredictionSnapshot snapshot{};
  bool valid = false;
  bool restored = false;

  explicit local_prediction_restore_guard(Player* localplayer)
    : player(localplayer) {
    valid = local_prediction_capture(player, &snapshot);
  }

  ~local_prediction_restore_guard() {
    if (valid && !restored) {
      local_prediction_restore(player, snapshot);
    }
  }

  bool restore_now() {
    if (!valid || restored) {
      return valid;
    }

    restored = local_prediction_restore(player, snapshot);
    return restored;
  }

  local_prediction_restore_guard(const local_prediction_restore_guard&) = delete;
  local_prediction_restore_guard& operator=(const local_prediction_restore_guard&) = delete;
};

struct local_prediction_host_guard {
  MoveHelper* helper = nullptr;

  explicit local_prediction_host_guard(MoveHelper* move_helper_in, Player* host) : helper(move_helper_in) {
    if (helper != nullptr) {
      helper->set_host(host);
    }
  }

  ~local_prediction_host_guard() {
    if (helper != nullptr) {
      helper->set_host(nullptr);
    }
  }

  local_prediction_host_guard(const local_prediction_host_guard&) = delete;
  local_prediction_host_guard& operator=(const local_prediction_host_guard&) = delete;
};

inline bool local_prediction_capture(Player* localplayer, LocalPredictionSnapshot* snapshot) {
  if (localplayer == nullptr || snapshot == nullptr || global_vars == nullptr || prediction == nullptr) return false;

  snapshot->player.origin = localplayer->get_origin();
  snapshot->player.abs_origin = localplayer->get_abs_origin();
  snapshot->player.velocity = localplayer->get_velocity();
  snapshot->player.base_velocity = localplayer->get_base_velocity();
  snapshot->player.view_offset = localplayer->get_view_offset();
  snapshot->player.flags = localplayer->get_flags();
  snapshot->player.ground_entity_handle = localplayer->get_ground_entity_handle();
  snapshot->player.tickbase = localplayer->get_tickbase();
  snapshot->player.current_cmd = localplayer->get_current_cmd();
  snapshot->player.ducked = localplayer->get_ducked();
  snapshot->player.ducking = localplayer->get_ducking_state();
  snapshot->player.in_duck_jump = localplayer->get_in_duck_jump();
  snapshot->player.duck_time = localplayer->get_duck_time();
  snapshot->player.duck_jump_time = localplayer->get_duck_jump_time();
  snapshot->player.fall_velocity = localplayer->get_fall_velocity();

  snapshot->globals.curtime = global_vars->curtime;
  snapshot->globals.frametime = global_vars->frametime;
  snapshot->globals.tickcount = global_vars->tickcount;
  snapshot->globals.first_time_predicted = prediction->first_time_predicted;
  snapshot->globals.in_prediction = prediction->in_prediction;
  snapshot->globals.random_seed_value = random_seed != nullptr ? static_cast<int>(*random_seed) : -1;
  return true;
}

inline bool local_prediction_restore(Player* localplayer, const LocalPredictionSnapshot& snapshot) {
  if (localplayer == nullptr || global_vars == nullptr || prediction == nullptr) return false;

  localplayer->set_origin(snapshot.player.origin);
  localplayer->set_abs_origin(snapshot.player.abs_origin);
  localplayer->set_velocity(snapshot.player.velocity);
  localplayer->set_base_velocity(snapshot.player.base_velocity);
  localplayer->set_view_offset(snapshot.player.view_offset);
  localplayer->set_flags(snapshot.player.flags);
  localplayer->set_ground_entity_handle(snapshot.player.ground_entity_handle);
  localplayer->set_tickbase(snapshot.player.tickbase);
  localplayer->set_current_cmd(snapshot.player.current_cmd);
  localplayer->set_ducked(snapshot.player.ducked);
  localplayer->set_ducking_state(snapshot.player.ducking);
  localplayer->set_in_duck_jump(snapshot.player.in_duck_jump);
  localplayer->set_duck_time(snapshot.player.duck_time);
  localplayer->set_duck_jump_time(snapshot.player.duck_jump_time);
  localplayer->set_fall_velocity(snapshot.player.fall_velocity);

  global_vars->curtime = snapshot.globals.curtime;
  global_vars->frametime = snapshot.globals.frametime;
  global_vars->tickcount = snapshot.globals.tickcount;
  prediction->first_time_predicted = snapshot.globals.first_time_predicted;
  prediction->in_prediction = snapshot.globals.in_prediction;
  if (random_seed != nullptr) {
    *random_seed = snapshot.globals.random_seed_value;
  }
  return true;
}

inline bool local_prediction_run(Player* localplayer, const LocalPredictionRequest& request, LocalPredictionResult* result) {
  static bool local_prediction_active = false;
  local_prediction_debug = {};
  if (result != nullptr) {
    *result = {};
  }
  if (localplayer == nullptr || prediction == nullptr || game_movement == nullptr || move_helper == nullptr || global_vars == nullptr) {
    local_prediction_debug.last_failure_stage = 1;
    return false;
  }
  if (request.ticks.empty() || local_prediction_active) {
    local_prediction_debug.last_failure_stage = 2;
    return false;
  }

  local_prediction_restore_guard restore_guard(localplayer);
  if (!restore_guard.valid) {
    local_prediction_debug.last_failure_stage = 3;
    return false;
  }

  local_prediction_bool_guard active_guard(&local_prediction_active, true);

  for (size_t tick_index = 0; tick_index < request.ticks.size(); ++tick_index) {
    const LocalPredictionTick& step = request.ticks[tick_index];
    user_cmd cmd = step.cmd;
    if (cmd.command_number == 0) {
      cmd.command_number = restore_guard.snapshot.globals.tickcount + static_cast<int>(tick_index) + 1;
    }
    if (cmd.tick_count <= 0) {
      cmd.tick_count = restore_guard.snapshot.globals.tickcount + static_cast<int>(tick_index) + 1;
    }
    cmd.has_been_predicted = false;

    localplayer->set_tickbase(restore_guard.snapshot.player.tickbase + static_cast<int>(tick_index));
    localplayer->set_current_cmd(&cmd);
    if (random_seed != nullptr) {
      *random_seed = MD5_PseudoRandom(static_cast<unsigned int>(cmd.command_number)) & INT_MAX;
    }

    global_vars->curtime = localplayer->get_tickbase() * TICK_INTERVAL;
    global_vars->frametime = prediction->engine_paused ? 0.0f : TICK_INTERVAL;
    global_vars->tickcount = localplayer->get_tickbase();
    prediction->first_time_predicted = false;
    prediction->in_prediction = true;
    prediction->set_local_view_angles(cmd.view_angles);

    move_helper->set_host(localplayer);
    prediction->run_command(localplayer, &cmd, move_helper);
    move_helper->set_host(nullptr);

    if (result != nullptr) {
      LocalPredictionTickResult tick_result{};
      tick_result.cmd = cmd;
      tick_result.origin = localplayer->get_origin();
      tick_result.velocity = localplayer->get_velocity();
      tick_result.grounded = localplayer->is_on_ground();
      result->ticks.push_back(tick_result);
      result->simulated_ticks = static_cast<int>(result->ticks.size());
    }
  }

  local_prediction_debug.last_simulated_ticks = static_cast<int>(request.ticks.size());
  local_prediction_debug.last_restore_ok = restore_guard.restore_now();
  if (result != nullptr) {
    result->success = local_prediction_debug.last_restore_ok;
    result->restored = local_prediction_debug.last_restore_ok;
  }
  local_prediction_debug.last_run_ok = local_prediction_debug.last_restore_ok;
  return local_prediction_debug.last_restore_ok;
}

inline float local_prediction_vector_length(Vec3 value) {
  return std::sqrt((value.x * value.x) + (value.y * value.y) + (value.z * value.z));
}

inline Vec3 local_prediction_normalize(Vec3 value) {
  float length = local_prediction_vector_length(value);
  if (length <= 0.0001f) return Vec3{};
  return value * (1.0f / length);
}

inline Vec3 local_prediction_direction_to_angles(const Vec3& direction) {
  float yaw_hyp = std::sqrt((direction.x * direction.x) + (direction.y * direction.y));
  return Vec3{
    -std::atan2(direction.z, yaw_hyp) * radpi,
    std::atan2(direction.y, direction.x) * radpi,
    0.0f
  };
}

inline Vec3 local_prediction_angles_to_direction(const Vec3& angles) {
  float pitch = angles.x * pideg;
  float yaw = angles.y * pideg;
  float cp = std::cos(pitch);
  return Vec3{
    cp * std::cos(yaw),
    cp * std::sin(yaw),
    -std::sin(pitch)
  };
}

inline float local_prediction_projectile_gravity_scale() {
  static Convar* sv_gravity = nullptr;
  if (sv_gravity == nullptr && convar_system != nullptr) {
    sv_gravity = convar_system->find_var("sv_gravity");
  }

  const float gravity = sv_gravity != nullptr ? sv_gravity->get_float() : 800.0f;
  return gravity / 800.0f;
}

inline bool local_prediction_vec3_is_zero(const Vec3& value) {
  return value.x == 0.0f && value.y == 0.0f && value.z == 0.0f;
}

struct LocalPredictionEntityHistorySample {
  Vec3 origin{};
  Vec3 velocity{};
  float sim_time = 0.0f;
  float curtime = 0.0f;
  int flags = 0;
  bool ducking = false;
  bool on_ground = false;
};

struct LocalPredictionEntityHistory {
  int ent_index = 0;
  int sample_count = 0;
  std::array<LocalPredictionEntityHistorySample, 48> samples{};
};

inline static std::array<LocalPredictionEntityHistory, 2049> local_prediction_entity_history{};

inline void local_prediction_record_entity(Entity* entity) {
  if (entity == nullptr || global_vars == nullptr) return;
  int ent_index = entity->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) return;

  LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  LocalPredictionEntityHistorySample sample{};
  sample.origin = entity->get_origin();
  sample.sim_time = entity->get_simulation_time();
  sample.curtime = global_vars->curtime;
  if (entity->get_class_id() == class_id::PLAYER) {
    Player* player = static_cast<Player*>(entity);
    sample.velocity = player->get_velocity();
    sample.flags = player->get_flags();
    sample.ducking = player->is_ducking();
    sample.on_ground = player->is_on_ground();
  }

  if (history.sample_count > 0) {
    const LocalPredictionEntityHistorySample& last = history.samples[0];
    if (last.sim_time == sample.sim_time && distance_squared_2d(last.origin, sample.origin) < 0.0001f &&
        std::fabs(last.origin.z - sample.origin.z) < 0.0001f) {
      return;
    }
  }

  history.ent_index = ent_index;
  int shift_count = std::min(history.sample_count, static_cast<int>(history.samples.size()) - 1);
  for (int index = shift_count; index > 0; --index) {
    history.samples[index] = history.samples[index - 1];
  }
  history.samples[0] = sample;
  history.sample_count = std::min(history.sample_count + 1, static_cast<int>(history.samples.size()));
}

inline Vec3 local_prediction_estimate_entity_velocity(Entity* entity) {
  if (entity == nullptr) return Vec3{};
  int ent_index = entity->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) return Vec3{};

  const LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  if (history.sample_count < 2) return Vec3{};

  const auto pair_velocity = [](const LocalPredictionEntityHistorySample& newer,
                                const LocalPredictionEntityHistorySample& older,
                                Vec3* out) -> bool {
    float dt = newer.sim_time - older.sim_time;
    if (dt <= 0.0001f) {
      dt = newer.curtime - older.curtime;
    }
    if (dt <= 0.0001f) {
      return false;
    }
    *out = Vec3{
      (newer.origin.x - older.origin.x) / dt,
      (newer.origin.y - older.origin.y) / dt,
      (newer.origin.z - older.origin.z) / dt
    };
    return true;
  };

  Vec3 newest{};
  if (!pair_velocity(history.samples[0], history.samples[1], &newest)) {
    return Vec3{};
  }

  if (history.sample_count < 3) {
    return newest;
  }

  Vec3 prev{};
  if (!pair_velocity(history.samples[1], history.samples[2], &prev)) {
    return newest;
  }

  return Vec3{
    (newest.x * 0.75f) + (prev.x * 0.25f),
    (newest.y * 0.75f) + (prev.y * 0.25f),
    (newest.z * 0.75f) + (prev.z * 0.25f)
  };
}

struct LocalPredictionEntityPath {
  bool valid = false;
  bool used_movement_sim = false;
  bool used_game_engine_movement = false;
  bool used_strafe_prediction = false;
  float start_time = 0.0f;
  float time_step = TICK_INTERVAL;
  float average_yaw = 0.0f;
  float strafe_confidence = 0.0f;
  Vec3 final_velocity{};
  std::vector<Vec3> positions{};
};

inline float local_prediction_velocity_2d_length(const Vec3& velocity) {
  return std::sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y));
}

inline float local_prediction_normalize_yaw(float yaw) {
  return std::remainder(yaw, 360.0f);
}

inline float local_prediction_vector_yaw(const Vec3& velocity) {
  return std::atan2(velocity.y, velocity.x) * radpi;
}

inline float local_prediction_estimate_sim_delta(Entity* entity) {
  if (entity == nullptr) {
    return TICK_INTERVAL;
  }

  int ent_index = entity->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) {
    return TICK_INTERVAL;
  }

  const LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  float sim_delta_sum = 0.0f;
  int sim_delta_count = 0;
  for (int index = 1; index < history.sample_count; ++index) {
    float delta = history.samples[index - 1].sim_time - history.samples[index].sim_time;
    if (delta > 0.0001f) {
      sim_delta_sum += delta;
      ++sim_delta_count;
    }
  }

  if (sim_delta_count <= 0) {
    return TICK_INTERVAL;
  }

  return std::clamp(sim_delta_sum / static_cast<float>(sim_delta_count), static_cast<float>(TICK_INTERVAL), 0.25f);
}

inline int local_prediction_time_to_ticks(float time) {
  const float tick_interval = global_vars != nullptr && global_vars->interval_per_tick > 0.0f
    ? global_vars->interval_per_tick
    : static_cast<float>(TICK_INTERVAL);
  return static_cast<int>(0.5f + (time / std::max(tick_interval, 0.0001f)));
}

inline float local_prediction_ticks_to_time(int ticks) {
  const float tick_interval = global_vars != nullptr && global_vars->interval_per_tick > 0.0f
    ? global_vars->interval_per_tick
    : static_cast<float>(TICK_INTERVAL);
  return static_cast<float>(ticks) * tick_interval;
}

inline float local_prediction_interp_time() {
  static Convar* cl_interp = nullptr;
  static Convar* cl_interp_ratio = nullptr;
  static Convar* cl_updaterate = nullptr;
  if (convar_system != nullptr) {
    if (cl_interp == nullptr) {
      cl_interp = convar_system->find_var("cl_interp");
    }
    if (cl_interp_ratio == nullptr) {
      cl_interp_ratio = convar_system->find_var("cl_interp_ratio");
    }
    if (cl_updaterate == nullptr) {
      cl_updaterate = convar_system->find_var("cl_updaterate");
    }
  }

  const float interp = cl_interp != nullptr ? cl_interp->get_float() : 0.0f;
  const float ratio = cl_interp_ratio != nullptr ? cl_interp_ratio->get_float() : 1.0f;
  const float update_rate = cl_updaterate != nullptr ? cl_updaterate->get_float() : 66.0f;
  return std::max(interp, ratio / std::max(update_rate, 1.0f));
}

inline float local_prediction_net_latency_flow(int flow) {
  if (client_state == nullptr || client_state->m_NetChannel == nullptr) {
    return 0.0f;
  }

  void** vtable = *reinterpret_cast<void***>(client_state->m_NetChannel);
  if (vtable == nullptr) {
    return 0.0f;
  }

  auto get_latency_fn = reinterpret_cast<float (*)(void*, int)>(vtable[9]);
  if (get_latency_fn == nullptr) {
    return 0.0f;
  }

  const float latency = get_latency_fn(client_state->m_NetChannel, flow);
  return std::isfinite(latency) ? std::clamp(latency, 0.0f, 1.0f) : 0.0f;
}

inline float local_prediction_outgoing_latency() {
  constexpr int flow_outgoing = 0;
  return local_prediction_net_latency_flow(flow_outgoing);
}

inline float local_prediction_incoming_latency() {
  constexpr int flow_incoming = 1;
  return local_prediction_net_latency_flow(flow_incoming);
}

inline float local_prediction_net_latency() {
  return std::clamp(local_prediction_outgoing_latency() + local_prediction_incoming_latency(), 0.0f, 1.0f);
}

inline float local_prediction_estimate_entity_lag(Entity* entity) {
  if (entity == nullptr || global_vars == nullptr) {
    return 0.0f;
  }

  const int ent_index = entity->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) {
    return 0.0f;
  }

  const LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  if (history.sample_count <= 0) {
    return 0.0f;
  }

  return std::clamp(global_vars->curtime - history.samples[0].sim_time, 0.0f, 0.6f);
}

inline float local_prediction_estimate_entity_choke_lag(Entity* entity) {
  if (entity == nullptr || global_vars == nullptr) {
    return 0.0f;
  }

  const int ent_index = entity->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) {
    return 0.0f;
  }

  const LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  if (history.sample_count <= 0) {
    return 0.0f;
  }

  const LocalPredictionEntityHistorySample& newest = history.samples[0];
  float choke_lag = std::max(global_vars->curtime - newest.curtime, 0.0f);
  if (history.sample_count >= 2) {
    const LocalPredictionEntityHistorySample& previous = history.samples[1];
    const float sim_delta = newest.sim_time - previous.sim_time;
    if (std::isfinite(sim_delta) && sim_delta > static_cast<float>(TICK_INTERVAL)) {
      choke_lag = std::max(choke_lag, sim_delta - static_cast<float>(TICK_INTERVAL));
    }
  }

  return std::clamp(choke_lag, 0.0f, 0.25f);
}

inline int local_prediction_network_lead_ticks(Entity* entity) {
  const float lead_time = local_prediction_outgoing_latency() +
    local_prediction_interp_time() +
    local_prediction_estimate_entity_choke_lag(entity);
  return std::clamp(
    static_cast<int>(local_prediction_time_to_ticks(lead_time)),
    0,
    std::clamp(config.aimbot.projectile_prediction_ticks, 8, 420) / 2);
}

inline float local_prediction_estimate_average_yaw_step(Player* player) {
  if (player == nullptr) {
    return 0.0f;
  }

  int ent_index = player->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) {
    return 0.0f;
  }

  const LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  float yaw_sum = 0.0f;
  int yaw_count = 0;

  for (int index = 1; index < history.sample_count; ++index) {
    const LocalPredictionEntityHistorySample& newer = history.samples[index - 1];
    const LocalPredictionEntityHistorySample& older = history.samples[index];
    if (newer.on_ground != older.on_ground) {
      continue;
    }

    const float newer_speed = local_prediction_velocity_2d_length(newer.velocity);
    const float older_speed = local_prediction_velocity_2d_length(older.velocity);
    if (newer_speed < 10.0f || older_speed < 10.0f) {
      continue;
    }

    float sim_delta = newer.sim_time - older.sim_time;
    if (sim_delta <= 0.0001f) {
      sim_delta = newer.curtime - older.curtime;
    }
    if (sim_delta <= 0.0001f) {
      continue;
    }

    const int tick_delta = std::max(1, static_cast<int>(std::round(sim_delta / TICK_INTERVAL)));
    const float newer_yaw = local_prediction_vector_yaw(newer.velocity);
    const float older_yaw = local_prediction_vector_yaw(older.velocity);
    const float yaw_delta = local_prediction_normalize_yaw(newer_yaw - older_yaw) / static_cast<float>(tick_delta);
    if (std::fabs(yaw_delta) > 45.0f) {
      continue;
    }

    yaw_sum += yaw_delta;
    ++yaw_count;
  }

  if (yaw_count < 2) {
    return 0.0f;
  }

  return yaw_sum / static_cast<float>(yaw_count);
}

struct local_prediction_strafe_estimate {
  float yaw_step = 0.0f;
  float confidence = 0.0f;
  bool valid = false;
};

inline float local_prediction_dot_2d(const Vec3& left, const Vec3& right) {
  return (left.x * right.x) + (left.y * right.y);
}

inline Vec3 local_prediction_yaw_to_forward(float yaw) {
  const float radians = yaw * pideg;
  return Vec3{std::cos(radians), std::sin(radians), 0.0f};
}

inline Vec3 local_prediction_rotate_2d(const Vec3& value, float yaw_delta) {
  const float radians = yaw_delta * pideg;
  const float sine = std::sin(radians);
  const float cosine = std::cos(radians);
  return Vec3{
    (value.x * cosine) - (value.y * sine),
    (value.x * sine) + (value.y * cosine),
    value.z
  };
}

inline local_prediction_strafe_estimate local_prediction_estimate_strafe(Player* player) {
  local_prediction_strafe_estimate estimate{};
  if (player == nullptr) {
    return estimate;
  }

  const int ent_index = player->get_index();
  if (ent_index <= 0 || ent_index >= static_cast<int>(local_prediction_entity_history.size())) {
    return estimate;
  }

  const LocalPredictionEntityHistory& history = local_prediction_entity_history[ent_index];
  if (history.sample_count < 5) {
    return estimate;
  }

  const bool current_air = !history.samples[0].on_ground;
  const float current_speed = local_prediction_velocity_2d_length(history.samples[0].velocity);
  const int wanted_samples = current_air ? 14 : 10;
  const float straight_fuzzy_value = current_air ? 56.0f : 84.0f;
  const int max_sign_changes = current_air ? 2 : 1;
  const int min_ticks_since_last_flip = current_air ? 3 : 2;

  float streak_yaw_sum = 0.0f;
  int streak_yaw_ticks = 0;
  int streak_pairs = 0;
  int streak_sign = 0;
  int sign_changes_seen = 0;
  int ticks_since_last_flip = 0;
  bool flipped_too_recently = false;

  const int sample_limit = std::min(history.sample_count, wanted_samples);
  for (int index = 1; index < sample_limit; ++index) {
    const auto& newer = history.samples[index - 1];
    const auto& older = history.samples[index];
    if (newer.on_ground != older.on_ground) {
      break;
    }

    const float newer_speed = local_prediction_velocity_2d_length(newer.velocity);
    const float older_speed = local_prediction_velocity_2d_length(older.velocity);
    if (newer_speed < 40.0f || older_speed < 40.0f) {
      break;
    }

    float delta_time = newer.sim_time - older.sim_time;
    if (delta_time <= 0.0001f) {
      delta_time = newer.curtime - older.curtime;
    }
    if (delta_time <= 0.0001f) {
      continue;
    }

    const int tick_delta = std::max(1, static_cast<int>(std::round(delta_time / TICK_INTERVAL)));
    const float yaw_delta = local_prediction_normalize_yaw(
      local_prediction_vector_yaw(newer.velocity) - local_prediction_vector_yaw(older.velocity));
    const float yaw_per_tick = yaw_delta / static_cast<float>(tick_delta);
    if (std::fabs(yaw_per_tick) > 45.0f) {
      break;
    }

    const bool straight = std::fabs(yaw_delta) * newer_speed * static_cast<float>(tick_delta) < straight_fuzzy_value;
    if (straight) {
      // Treat straight motion as a flip in counting -- it breaks the strafe streak.
      ++sign_changes_seen;
      if (sign_changes_seen > max_sign_changes) {
        break;
      }
      if (streak_sign == 0 && ticks_since_last_flip < min_ticks_since_last_flip) {
        flipped_too_recently = true;
      }
      streak_sign = 0;
      continue;
    }

    const int pair_sign = yaw_per_tick > 0.0f ? 1 : -1;
    if (streak_sign != 0 && pair_sign != streak_sign) {
      if (ticks_since_last_flip < min_ticks_since_last_flip) {
        flipped_too_recently = true;
      }
      break;
    }

    streak_sign = pair_sign;
    streak_yaw_sum += yaw_per_tick * static_cast<float>(tick_delta);
    streak_yaw_ticks += tick_delta;
    ticks_since_last_flip += tick_delta;
    ++streak_pairs;
  }

  if (streak_pairs < 2 || streak_yaw_ticks <= 0 || flipped_too_recently) {
    return estimate;
  }

  estimate.yaw_step = streak_yaw_sum / static_cast<float>(streak_yaw_ticks);
  estimate.confidence = std::clamp(
    (static_cast<float>(streak_pairs) / static_cast<float>(std::max(2, sample_limit - 1))) * 100.0f,
    0.0f,
    100.0f);

  constexpr float required_confidence = 55.0f;

  estimate.valid = std::fabs(estimate.yaw_step) >= 0.10f &&
    estimate.confidence >= required_confidence;
  return estimate;
}

inline float local_prediction_estimated_max_speed(Player* player, const Vec3& velocity) {
  if (player == nullptr) {
    return 300.0f;
  }

  constexpr float max_prediction_speed = 3500.0f;
  const float observed_speed = local_prediction_velocity_2d_length(velocity);
  const float netvar_max_speed = player->get_max_speed();
  if (netvar_max_speed > 1.0f && netvar_max_speed < 1000.0f) {
    return std::clamp(std::max(netvar_max_speed, observed_speed), 1.0f, max_prediction_speed);
  }

  float base_speed = 300.0f;
  switch (player->get_tf_class()) {
  case tf_class::SCOUT:
    base_speed = 400.0f;
    break;
  case tf_class::SOLDIER:
    base_speed = 240.0f;
    break;
  case tf_class::DEMOMAN:
    base_speed = 280.0f;
    break;
  case tf_class::MEDIC:
  case tf_class::SPY:
    base_speed = 320.0f;
    break;
  case tf_class::HEAVYWEAPONS:
    base_speed = 230.0f;
    break;
  default:
    break;
  }

  return std::clamp(std::max(base_speed, observed_speed), 1.0f, max_prediction_speed);
}

inline void local_prediction_fill_move_from_velocity(MoveData* move, const Vec3& velocity, float view_yaw, float max_speed) {
  if (move == nullptr) {
    return;
  }

  const Vec3 forward = local_prediction_yaw_to_forward(view_yaw);
  const Vec3 right{-forward.y, forward.x, 0.0f};
  Vec3 wish_velocity{velocity.x, velocity.y, 0.0f};
  const float wish_speed = local_prediction_velocity_2d_length(wish_velocity);
  if (wish_speed > max_speed && wish_speed > 0.0001f) {
    wish_velocity = wish_velocity * (max_speed / wish_speed);
  }

  move->m_flForwardMove = std::clamp(local_prediction_dot_2d(wish_velocity, forward), -max_speed, max_speed);
  move->m_flSideMove = std::clamp(local_prediction_dot_2d(wish_velocity, right), -max_speed, max_speed);
  move->m_flUpMove = 0.0f;
}

inline int local_prediction_configured_path_ticks() {
  return std::clamp(config.aimbot.projectile_prediction_ticks, 8, 420);
}

inline int local_prediction_path_tick_count(float horizon_seconds) {
  return std::clamp(
    static_cast<int>(std::ceil(horizon_seconds / TICK_INTERVAL)),
    1,
    local_prediction_configured_path_ticks());
}

inline Vec3 local_prediction_player_path_seed_velocity(Player* player);

inline bool local_prediction_simulate_player_path(Player* player,
  float horizon_seconds,
  LocalPredictionEntityPath* path_out,
  int lead_ticks = 0,
  int max_sim_ticks = INT_MAX) {
  if (player == nullptr || path_out == nullptr || prediction == nullptr || game_movement == nullptr || move_helper == nullptr || global_vars == nullptr) {
    return false;
  }

  if (horizon_seconds <= 0.0f || max_sim_ticks <= 0) {
    return false;
  }

  static bool movement_sim_active = false;
  if (movement_sim_active) {
    return false;
  }

  local_prediction_restore_guard restore_guard(player);
  if (!restore_guard.valid) {
    return false;
  }

  local_prediction_bool_guard active_guard(&movement_sim_active, true);

  Vec3 initial_velocity = restore_guard.snapshot.player.velocity;
  const bool is_local_player = entity_list != nullptr && player == entity_list->get_localplayer();
  if (!is_local_player) {
    initial_velocity = local_prediction_player_path_seed_velocity(player);
  }

  *path_out = {};
  path_out->time_step = TICK_INTERVAL;
  path_out->start_time = local_prediction_ticks_to_time(lead_ticks);
  path_out->used_movement_sim = true;
  path_out->used_game_engine_movement = true;
  const int requested_tick_count = local_prediction_path_tick_count(horizon_seconds);
  int tick_count = std::min(requested_tick_count, max_sim_ticks);
  tick_count = std::clamp(tick_count, 1, requested_tick_count);
  path_out->positions.reserve(static_cast<size_t>(tick_count) + 1);
  if (lead_ticks <= 0) {
    path_out->positions.push_back(restore_guard.snapshot.player.origin);
  }

  MoveData move{};
  move.m_bFirstRunOfFunctions = false;
  move.m_bGameCodeMovedPlayer = false;
  move.m_nPlayerHandle = player->get_ref_handle();
  move.SetAbsOrigin(restore_guard.snapshot.player.origin);
  move.m_vecVelocity = initial_velocity;

  player->set_velocity(initial_velocity);
  if (player->is_ducking()) {
    player->set_ducked(true);
    player->set_flags(player->get_flags() & ~FL_DUCKING);
    player->set_duck_time(0.0f);
    player->set_duck_jump_time(0.0f);
    player->set_ducking_state(false);
    player->set_in_duck_jump(false);
  }

  if (!is_local_player) {
    player->set_base_velocity(Vec3{});
    if (restore_guard.snapshot.player.flags & FL_ONGROUND) {
      move.m_vecVelocity.z = std::min(move.m_vecVelocity.z, 0.0f);
      player->set_velocity(move.m_vecVelocity);
    } else {
      player->set_ground_entity_handle(-1);
    }
  }

  const float observed_speed = local_prediction_velocity_2d_length(initial_velocity);
  const float max_speed = local_prediction_estimated_max_speed(player, initial_velocity);
  move.m_flMaxSpeed = max_speed;
  move.m_flClientMaxSpeed = max_speed;

  float view_yaw = observed_speed > 1.0f ? local_prediction_vector_yaw(initial_velocity) : player->get_eye_angles().y;
  const local_prediction_strafe_estimate strafe_estimate = local_prediction_estimate_strafe(player);
  const float yaw_step = strafe_estimate.valid ? strafe_estimate.yaw_step : local_prediction_estimate_average_yaw_step(player);
  path_out->average_yaw = yaw_step;
  path_out->strafe_confidence = strafe_estimate.confidence;
  path_out->used_strafe_prediction = strafe_estimate.valid;

  move.m_vecViewAngles = {0.0f, view_yaw, 0.0f};
  move.m_vecAbsViewAngles = move.m_vecViewAngles;
  move.m_vecAngles = move.m_vecViewAngles;
  move.m_vecOldAngles = move.m_vecViewAngles;
  move.m_nButtons = player->get_buttons();
  move.m_nOldButtons = player->get_last_buttons();
  if (restore_guard.snapshot.player.ducked || player->get_ducked()) {
    move.m_nButtons |= IN_DUCK;
  }
  local_prediction_fill_move_from_velocity(&move, initial_velocity, view_yaw, max_speed);

  Entity* constraint_entity = player->get_constraint_entity();
  move.m_vecConstraintCenter = constraint_entity != nullptr ? constraint_entity->get_abs_origin() : player->get_constraint_center();
  move.m_flConstraintRadius = player->get_constraint_radius();
  move.m_flConstraintWidth = player->get_constraint_width();
  move.m_flConstraintSpeedFactor = player->get_constraint_speed_factor();

  user_cmd dummy_cmd{};
  dummy_cmd.command_number = restore_guard.snapshot.globals.tickcount + 1;
  dummy_cmd.tick_count = restore_guard.snapshot.globals.tickcount + 1;
  dummy_cmd.buttons = move.m_nButtons;

  local_prediction_host_guard host_guard(move_helper, player);
  player->set_current_cmd(&dummy_cmd);

  bool movement_ok = true;
  for (int tick = 0; tick < tick_count + lead_ticks; ++tick) {
    global_vars->curtime = (restore_guard.snapshot.player.tickbase + tick) * TICK_INTERVAL;
    if (!is_local_player) {
      global_vars->curtime = restore_guard.snapshot.globals.curtime + (static_cast<float>(tick) * static_cast<float>(TICK_INTERVAL));
    }
    global_vars->frametime = prediction->engine_paused ? 0.0f : TICK_INTERVAL;
    global_vars->tickcount = restore_guard.snapshot.globals.tickcount + tick;
    prediction->first_time_predicted = false;
    prediction->in_prediction = true;

    if (std::fabs(yaw_step) > 0.01f) {
      view_yaw = local_prediction_normalize_yaw(view_yaw + yaw_step);
      move.m_vecVelocity = local_prediction_rotate_2d(move.m_vecVelocity, yaw_step);
      local_prediction_fill_move_from_velocity(&move, move.m_vecVelocity, view_yaw, max_speed);
    } else if (!player->is_on_ground() && observed_speed <= 1.0f) {
      move.m_flForwardMove = 0.0f;
      move.m_flSideMove = 0.0f;
    }

    move.m_vecViewAngles = {0.0f, view_yaw, 0.0f};
    move.m_vecAbsViewAngles = move.m_vecViewAngles;
    move.m_vecAngles = move.m_vecViewAngles;
    move.m_vecOldAngles = move.m_vecViewAngles;
    dummy_cmd.view_angles = move.m_vecViewAngles;
    dummy_cmd.buttons = move.m_nButtons;
    dummy_cmd.command_number = restore_guard.snapshot.globals.tickcount + tick + 1;
    dummy_cmd.tick_count = restore_guard.snapshot.globals.tickcount + tick + 1;

    const float old_client_max_speed = move.m_flClientMaxSpeed;
    if (player->get_ducked() && player->is_on_ground() && player->get_water_level() <= 1) {
      move.m_flClientMaxSpeed = old_client_max_speed / 3.0f;
    }

    if (!game_movement->process_movement(player, &move)) {
      move.m_flClientMaxSpeed = old_client_max_speed;
      movement_ok = false;
      break;
    }
    move.m_flClientMaxSpeed = old_client_max_speed;

    player->set_origin(move.GetAbsOrigin());
    player->set_abs_origin(move.GetAbsOrigin());
    player->set_velocity(move.m_vecVelocity);
    if (tick + 1 >= lead_ticks) {
      path_out->positions.push_back(move.GetAbsOrigin());
    }
  }

  path_out->final_velocity = move.m_vecVelocity;
  path_out->valid = movement_ok && path_out->positions.size() > 1;

  restore_guard.restore_now();
  return path_out->valid;
}


inline float local_prediction_dot_3d(const Vec3& left, const Vec3& right) {
  return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

inline Vec3 local_prediction_clip_velocity(const Vec3& velocity, const Vec3& normal, float overbounce = 1.0f) {
  const float backoff = local_prediction_dot_3d(velocity, normal) * overbounce;
  Vec3 clipped = velocity - (normal * backoff);
  if (std::fabs(clipped.x) < 0.001f) clipped.x = 0.0f;
  if (std::fabs(clipped.y) < 0.001f) clipped.y = 0.0f;
  if (std::fabs(clipped.z) < 0.001f) clipped.z = 0.0f;
  return clipped;
}

struct local_prediction_lightweight_state {
  Vec3 origin{};
  Vec3 velocity{};
  Vec3 mins{};
  Vec3 maxs{};
  bool grounded = false;
  bool swimming = false;
  bool has_move_input = false;
};

inline bool local_prediction_trace_hull(const Vec3& start,
  const Vec3& end,
  const Vec3& mins,
  const Vec3& maxs,
  Entity* skip_entity,
  trace_t* trace_out) {
  if (engine_trace == nullptr || trace_out == nullptr) {
    return false;
  }

  ray_t ray = engine_trace->init_ray(const_cast<Vec3*>(&start), const_cast<Vec3*>(&end), const_cast<Vec3*>(&mins), const_cast<Vec3*>(&maxs));
  trace_filter filter{};
  engine_trace->init_trace_filter(&filter, skip_entity);
  engine_trace->trace_ray(&ray, MASK_PLAYERSOLID, &filter, trace_out);
  return trace_out->fraction < 1.0f || trace_out->start_solid || trace_out->all_solid;
}

inline bool local_prediction_find_ground(local_prediction_lightweight_state* state, Entity* skip_entity) {
  if (state == nullptr) {
    return false;
  }

  // do not snap a rising target back to the floor; jumping/rocket-jumping/airblasted players have
  // upward z velocity and treating them as grounded zeros that velocity, ruining airborne prediction
  constexpr float upward_velocity_epsilon = 16.0f;
  if (state->velocity.z > upward_velocity_epsilon) {
    return false;
  }

  // shorten the probe when the target is clearly mid-air; only snap when actually settling on a surface
  const float ground_probe = state->grounded ? 18.0f : 4.0f;
  const Vec3 end = state->origin - Vec3{0.0f, 0.0f, ground_probe};
  trace_t trace{};
  if (!local_prediction_trace_hull(state->origin, end, state->mins, state->maxs, skip_entity, &trace)) {
    return false;
  }

  if (trace.start_solid || trace.all_solid || trace.plane.normal.z < 0.65f) {
    return false;
  }

  if (trace.fraction >= 1.0f) {
    return false;
  }

  state->origin = trace.endpos;
  state->velocity.z = 0.0f;
  return true;
}

inline Vec3 local_prediction_client_mv_horizontal_wish_dir(const local_prediction_lightweight_state* state, float move_yaw_deg) {
  if (state == nullptr) {
    return Vec3{1.0f, 0.0f, 0.0f};
  }

  const float speed_2d = local_prediction_velocity_2d_length(state->velocity);
  if (speed_2d > 8.0f) {
    return Vec3{state->velocity.x / speed_2d, state->velocity.y / speed_2d, 0.0f};
  }

  return local_prediction_yaw_to_forward(move_yaw_deg);
}

inline float local_prediction_client_mv_sv_gravity() {
  static Convar* sv_gravity = nullptr;
  if (sv_gravity == nullptr && convar_system != nullptr) {
    sv_gravity = convar_system->find_var("sv_gravity");
  }
  const float g = sv_gravity != nullptr ? sv_gravity->get_float() : 800.0f;
  return std::clamp(g, 100.0f, 2000.0f);
}

inline float local_prediction_client_mv_sv_friction() {
  static Convar* v = nullptr;
  if (v == nullptr && convar_system != nullptr) {
    v = convar_system->find_var("sv_friction");
  }
  return v != nullptr ? std::clamp(v->get_float(), 0.0f, 10.0f) : 1.0f;
}

inline float local_prediction_client_mv_sv_stopspeed() {
  static Convar* v = nullptr;
  if (v == nullptr && convar_system != nullptr) {
    v = convar_system->find_var("sv_stopspeed");
  }
  return v != nullptr ? std::clamp(v->get_float(), 1.0f, 1000.0f) : 100.0f;
}

inline float local_prediction_client_mv_sv_airaccelerate() {
  static Convar* v = nullptr;
  if (v == nullptr && convar_system != nullptr) {
    v = convar_system->find_var("sv_airaccelerate");
  }
  return v != nullptr ? std::clamp(v->get_float(), 0.0f, 150.0f) : 10.0f;
}

// i dont know tf2 says this in the source :thumbsup: white heart white heart
inline float local_prediction_client_mv_get_air_speed_cap() {
  return 30.0f;
}

inline float local_prediction_air_max_yaw_rate_per_tick(float horizontal_speed, float max_speed, float dt) {
  if (horizontal_speed <= 1.0f || dt <= 0.0f) {
    return 30.0f;
  }
  const float airaccel = local_prediction_client_mv_sv_airaccelerate();
  const float air_cap = local_prediction_client_mv_get_air_speed_cap();
  const float wish_for_accel = std::max(max_speed, 1.0f);
  const float accel_speed = airaccel * wish_for_accel * dt;
  const float max_perp = std::min(accel_speed, air_cap);
  if (max_perp <= 0.0f) {
    return 0.0f;
  }
  const float radians = std::atan2(max_perp, horizontal_speed);
  return radians / pideg;
}

inline void local_prediction_client_mv_apply_friction(local_prediction_lightweight_state* state, float dt) {
  if (state == nullptr || !state->grounded || state->swimming || dt <= 0.0f) {
    return;
  }

  const float speed = local_prediction_velocity_2d_length(state->velocity);
  if (speed < 0.1f) {
    return;
  }

  constexpr float surface_friction = 1.0f;
  const float friction = local_prediction_client_mv_sv_friction() * surface_friction;
  const float stop_speed = local_prediction_client_mv_sv_stopspeed();
  const float control = speed < stop_speed ? stop_speed : speed;
  const float new_speed = speed - (dt * control * friction);
  if (new_speed <= 0.0f) {
    state->velocity.x = 0.0f;
    state->velocity.y = 0.0f;
    return;
  }

  const float scale = new_speed / speed;
  state->velocity.x *= scale;
  state->velocity.y *= scale;
}

inline float local_prediction_client_mv_sv_accelerate() {
  static Convar* v = nullptr;
  if (v == nullptr && convar_system != nullptr) {
    v = convar_system->find_var("sv_accelerate");
  }
  return v != nullptr ? std::clamp(v->get_float(), 0.0f, 50.0f) : 10.0f;
}

inline void local_prediction_client_mv_accelerate_along_wish(
  local_prediction_lightweight_state* state,
  float dt,
  float max_speed,
  float accel_scale,
  const Vec3& wish_dir,
  float wish_speed_cap = 0.0f) {
  if (state == nullptr || dt <= 0.0f || accel_scale <= 0.0f) {
    return;
  }

  const float wish_speed = max_speed;
  const float wish_speed_for_add = wish_speed_cap > 0.0f
    ? std::min(wish_speed, wish_speed_cap)
    : wish_speed;
  const float current_speed = local_prediction_dot_2d(state->velocity, wish_dir);
  const float add_speed = wish_speed_for_add - current_speed;
  if (add_speed <= 0.0f) {
    return;
  }

  float accel_speed = accel_scale * wish_speed * dt;
  if (accel_speed > add_speed) {
    accel_speed = add_speed;
  }

  state->velocity.x += wish_dir.x * accel_speed;
  state->velocity.y += wish_dir.y * accel_speed;

  if (wish_speed_cap > 0.0f) {
    return;
  }
  const float new_2d = local_prediction_velocity_2d_length(state->velocity);
  const float cap = max_speed * 1.15f;
  if (new_2d > cap && new_2d > 0.001f) {
    const float scale = cap / new_2d;
    state->velocity.x *= scale;
    state->velocity.y *= scale;
  }
}

inline void local_prediction_player_path_tick(local_prediction_lightweight_state* state,
  Entity* skip_entity,
  float dt,
  float yaw_step,
  bool with_hull_trace,
  float max_speed,
  float* move_yaw_deg) {
  if (state == nullptr || skip_entity == nullptr || dt <= 0.0f || move_yaw_deg == nullptr) {
    return;
  }

  if (state->grounded && !state->swimming && state->velocity.z > 16.0f) {
    state->grounded = false;
  }

  if (!state->grounded && !state->swimming) {
    const float h_speed = local_prediction_velocity_2d_length(state->velocity);
    const float max_air_yaw = local_prediction_air_max_yaw_rate_per_tick(h_speed, max_speed, dt);
    if (max_air_yaw > 0.0f) {
      if (yaw_step > max_air_yaw) yaw_step = max_air_yaw;
      if (yaw_step < -max_air_yaw) yaw_step = -max_air_yaw;
    }
  }

  local_prediction_client_mv_apply_friction(state, dt);

  if (std::fabs(yaw_step) > 0.01f && local_prediction_velocity_2d_length(state->velocity) > 20.0f) {
    state->velocity = local_prediction_rotate_2d(state->velocity, yaw_step);
  }

  *move_yaw_deg = local_prediction_normalize_yaw(*move_yaw_deg + yaw_step);

  const Vec3 wish_dir = local_prediction_client_mv_horizontal_wish_dir(state, *move_yaw_deg);

  if (state->has_move_input && state->grounded && !state->swimming) {
    local_prediction_client_mv_accelerate_along_wish(
      state,
      dt,
      max_speed,
      local_prediction_client_mv_sv_accelerate(),
      wish_dir,
      0.0f);
  } else if (state->has_move_input && !state->grounded && !state->swimming) {
    local_prediction_client_mv_accelerate_along_wish(
      state,
      dt,
      max_speed,
      local_prediction_client_mv_sv_airaccelerate(),
      wish_dir,
      local_prediction_client_mv_get_air_speed_cap());
  }

  const float gravity = state->swimming ? 0.0f : local_prediction_client_mv_sv_gravity();
  if (!state->grounded && !state->swimming) {
    state->velocity.z -= 0.5f * gravity * dt;  // StartGravity
  }

  Vec3 end = state->origin;
  if (state->grounded || state->swimming) {
    end += Vec3{state->velocity.x * dt, state->velocity.y * dt, 0.0f};
  } else {
    end += Vec3{
      state->velocity.x * dt,
      state->velocity.y * dt,
      state->velocity.z * dt
    };
  }

  if (!with_hull_trace) {
    state->origin = end;
    if (!state->grounded && !state->swimming) {
      state->velocity.z -= 0.5f * gravity * dt;  // FinishGravity
    }
    return;
  }

  const Vec3 saved_origin = state->origin;
  const Vec3 saved_velocity = state->velocity;
  const bool was_grounded = state->grounded;

  trace_t move_trace{};
  const bool any_block = local_prediction_trace_hull(state->origin, end, state->mins, state->maxs, skip_entity, &move_trace);

  if (any_block) {
    if (!move_trace.start_solid && !move_trace.all_solid) {
      state->origin = move_trace.endpos;
    }

    const float remaining = std::clamp(1.0f - move_trace.fraction, 0.0f, 1.0f);
    if (move_trace.plane.normal.z >= 0.65f) {
      state->grounded = true;
      state->velocity.z = 0.0f;
    } else {
      state->velocity = local_prediction_clip_velocity(state->velocity, move_trace.plane.normal, 1.0f);
      if (remaining > 0.001f) {
        const Vec3 slide_end = state->origin + (state->velocity * (dt * remaining));
        trace_t slide_trace{};
        if (!local_prediction_trace_hull(state->origin, slide_end, state->mins, state->maxs, skip_entity, &slide_trace)) {
          state->origin = slide_end;
        } else if (!slide_trace.start_solid && !slide_trace.all_solid) {
          state->origin = slide_trace.endpos;
        }
      }
      state->grounded = false;
    }
  } else {
    state->origin = end;
  }

  // step-up (Source CGameMovement::StepMove style): only fires when we were grounded coming into
  // the tick and the regular move was significantly blocked (saves 3 hull traces per tick when not
  // needed). Use whichever (slide vs step-up) gets us further horizontally.
  if (with_hull_trace && was_grounded && !state->swimming && any_block && move_trace.fraction < 0.85f) {
    constexpr float step_size = 18.0f;  // TF2 m_flStepSize
    constexpr float step_down_extra = 4.0f;
    trace_t up_trace{};
    const Vec3 up_end = saved_origin + Vec3{0.0f, 0.0f, step_size};
    local_prediction_trace_hull(saved_origin, up_end, state->mins, state->maxs, skip_entity, &up_trace);
    const Vec3 stepped = (up_trace.start_solid || up_trace.all_solid) ? saved_origin : up_trace.endpos;

    const Vec3 horiz_step = Vec3{saved_velocity.x * dt, saved_velocity.y * dt, 0.0f};
    const Vec3 forward_end = stepped + horiz_step;
    trace_t fwd_trace{};
    local_prediction_trace_hull(stepped, forward_end, state->mins, state->maxs, skip_entity, &fwd_trace);
    const Vec3 after_forward = (fwd_trace.start_solid || fwd_trace.all_solid) ? stepped : fwd_trace.endpos;

    trace_t down_trace{};
    const Vec3 down_end = after_forward - Vec3{0.0f, 0.0f, step_size + step_down_extra};
    if (local_prediction_trace_hull(after_forward, down_end, state->mins, state->maxs, skip_entity, &down_trace)) {
      if (!down_trace.start_solid && !down_trace.all_solid && down_trace.plane.normal.z >= 0.65f) {
        const float step_dx = down_trace.endpos.x - saved_origin.x;
        const float step_dy = down_trace.endpos.y - saved_origin.y;
        const float step_dist_2d_sq = (step_dx * step_dx) + (step_dy * step_dy);
        const float slide_dx = state->origin.x - saved_origin.x;
        const float slide_dy = state->origin.y - saved_origin.y;
        const float slide_dist_2d_sq = (slide_dx * slide_dx) + (slide_dy * slide_dy);
        if (step_dist_2d_sq > slide_dist_2d_sq + 1.0f) {
          state->origin = down_trace.endpos;
          state->velocity.x = saved_velocity.x;
          state->velocity.y = saved_velocity.y;
          state->velocity.z = 0.0f;
          state->grounded = true;
        }
      }
    }
  }

  // FinishGravity: second half of gravity, applied after the move so post-collision vz=0 stays sane
  if (!state->grounded && !state->swimming) {
    state->velocity.z -= 0.5f * gravity * dt;
  }

  if (!state->swimming) {
    state->grounded = local_prediction_find_ground(state, skip_entity);
  }
}

inline Vec3 local_prediction_player_path_seed_velocity(Player* player) {
  if (player == nullptr) {
    return Vec3{};
  }

  const Vec3 current_velocity = player->get_velocity();
  const float current_speed = local_prediction_velocity_2d_length(current_velocity);
  const bool current_stationary =
    player->is_on_ground() &&
    current_speed < 8.0f &&
    std::fabs(current_velocity.z) < 8.0f;
  if (current_stationary) {
    return Vec3{};
  }

  const bool is_local = entity_list != nullptr && player == entity_list->get_localplayer();
  if (is_local) {
    return current_velocity;
  }

  const Vec3 averaged = local_prediction_estimate_entity_velocity(player);
  const float averaged_speed = local_prediction_velocity_2d_length(averaged);
  if (averaged_speed > 8.0f || std::fabs(averaged.z) > 8.0f) {
    return averaged;
  }

  if (current_speed > 1.0f || std::fabs(current_velocity.z) > 1.0f) {
    return current_velocity;
  }

  return averaged;
}

inline LocalPredictionEntityPath local_prediction_build_player_path_client_sim(Player* player,
  Entity* skip_entity,
  int lead_ticks,
  int step_count,
  int hull_trace_stride) {
  LocalPredictionEntityPath path{};
  if (player == nullptr || skip_entity == nullptr || engine_trace == nullptr || step_count < 1) {
    return path;
  }

  Vec3 origin = player->get_origin();
  Vec3 velocity = local_prediction_player_path_seed_velocity(player);
  bool target_grounded = false;
  bool target_swimming = false;
  if (std::sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y) + (velocity.z * velocity.z)) <= 0.001f) {
    target_grounded = player->is_on_ground();
    target_swimming = player->get_water_level() > 1;
  } else {
    target_grounded = player->is_on_ground();
    target_swimming = player->get_water_level() > 1;
  }

  const int lead_clamped = std::max(0, lead_ticks);
  const float lead_time = local_prediction_ticks_to_time(lead_clamped);
  path.time_step = TICK_INTERVAL;
  path.start_time = lead_time;
  path.positions.reserve(static_cast<size_t>(step_count) + 1);

  local_prediction_lightweight_state state{};
  state.origin = origin;
  state.velocity = velocity;
  state.mins = player->get_player_mins(player->is_ducking());
  state.maxs = player->get_player_maxs(player->is_ducking());
  state.grounded = target_grounded || target_swimming;
  state.swimming = target_swimming;
  state.has_move_input = local_prediction_velocity_2d_length(velocity) > 8.0f;

// chinese z bump
  if (state.grounded) {
    constexpr float dist_epsilon_3 = 0.09375f;  // 0.03125f * 3
    state.origin.z += dist_epsilon_3;
  }

  if (engine_trace != nullptr) {
    const float seed_speed_2d = local_prediction_velocity_2d_length(state.velocity);
    if (seed_speed_2d > 50.0f) {
      const Vec3 probe_end = state.origin + (state.velocity * (static_cast<float>(TICK_INTERVAL) * 1.25f));
      trace_t probe_trace{};
      if (local_prediction_trace_hull(state.origin, probe_end, state.mins, state.maxs, skip_entity, &probe_trace) &&
          !probe_trace.start_solid && !probe_trace.all_solid &&
          probe_trace.fraction < 1.0f && probe_trace.plane.normal.z < 0.7f) {
        state.velocity = local_prediction_clip_velocity(state.velocity, probe_trace.plane.normal, 1.0f);
      }
    }
  }

  const local_prediction_strafe_estimate strafe_estimate = local_prediction_estimate_strafe(player);
  const float yaw_step = strafe_estimate.valid ? strafe_estimate.yaw_step : local_prediction_estimate_average_yaw_step(player);
  path.average_yaw = yaw_step;
  path.strafe_confidence = strafe_estimate.confidence;
  path.used_strafe_prediction = std::fabs(yaw_step) > 0.01f;
  path.used_movement_sim = true;
  path.used_game_engine_movement = false;

  const float max_speed_seed = local_prediction_estimated_max_speed(player, velocity);
  const float speed_2d = local_prediction_velocity_2d_length(state.velocity);
  if (speed_2d > max_speed_seed && speed_2d > 0.001f) {
    const float speed_scale = max_speed_seed / speed_2d;
    state.velocity.x *= speed_scale;
    state.velocity.y *= speed_scale;
  }

  float move_yaw = local_prediction_normalize_yaw(
    local_prediction_velocity_2d_length(velocity) > 8.0f ? local_prediction_vector_yaw(velocity) : player->get_eye_angles().y);

  const int stride = std::max(1, hull_trace_stride);
  int hull_step_counter = 0;
  for (int lead = 0; lead < lead_clamped; ++lead) {
    const bool hull_trace = (stride <= 1) || ((hull_step_counter % stride) == 0);
    const float max_speed_tick = local_prediction_estimated_max_speed(player, state.velocity);
    local_prediction_player_path_tick(
      &state,
      skip_entity,
      static_cast<float>(TICK_INTERVAL),
      yaw_step,
      hull_trace,
      max_speed_tick,
      &move_yaw);
    ++hull_step_counter;
  }
  path.positions.push_back(state.origin);

  for (int step = 1; step <= step_count; ++step) {
    const bool hull_trace = (stride <= 1) || ((hull_step_counter % stride) == 0);
    const float max_speed_tick = local_prediction_estimated_max_speed(player, state.velocity);
    local_prediction_player_path_tick(
      &state,
      skip_entity,
      static_cast<float>(TICK_INTERVAL),
      yaw_step,
      hull_trace,
      max_speed_tick,
      &move_yaw);
    ++hull_step_counter;
    path.positions.push_back(state.origin);
  }

  path.final_velocity = state.velocity;
  path.valid = path.positions.size() > 1;
  return path;
}

inline void local_prediction_extend_player_path_lightweight(Player* player,
  LocalPredictionEntityPath* path,
  int requested_tick_count) {
  if (player == nullptr || path == nullptr || !path->valid || path->positions.empty()) {
    return;
  }

  const int existing_tick_count = static_cast<int>(path->positions.size()) - 1;
  if (existing_tick_count >= requested_tick_count) {
    return;
  }

  local_prediction_lightweight_state state{};
  state.origin = path->positions.back();
  state.velocity = path->final_velocity;
  state.mins = player->get_player_mins(player->is_ducking());
  state.maxs = player->get_player_maxs(player->is_ducking());
  state.swimming = player->get_water_level() > 1;
  state.grounded = state.swimming || (std::fabs(state.velocity.z) <= 1.0f && player->is_on_ground());
  state.has_move_input = local_prediction_velocity_2d_length(state.velocity) > 8.0f;

  path->positions.reserve(static_cast<size_t>(requested_tick_count) + 1);
  constexpr int trace_stride = 2;
  float move_yaw = local_prediction_normalize_yaw(
    local_prediction_velocity_2d_length(state.velocity) > 8.0f
      ? local_prediction_vector_yaw(state.velocity)
      : player->get_eye_angles().y);
  for (int step = existing_tick_count + 1; step <= requested_tick_count; ++step) {
    const int steps_since_extend = step - (existing_tick_count + 1);
    const bool hull_trace = (trace_stride <= 1) || ((steps_since_extend % trace_stride) == 0);
    const float max_speed_tick = local_prediction_estimated_max_speed(player, state.velocity);
    local_prediction_player_path_tick(
      &state,
      player,
      static_cast<float>(TICK_INTERVAL),
      path->average_yaw,
      hull_trace,
      max_speed_tick,
      &move_yaw);
    path->positions.push_back(state.origin);
  }

  path->final_velocity = state.velocity;
  path->valid = path->positions.size() > 1;
}

inline LocalPredictionEntityPath local_prediction_predict_entity_path(Entity* entity,
  float horizon_seconds,
  bool use_movement_sim = true,
  bool apply_network_lead = false) {
  LocalPredictionEntityPath path{};
  if (entity == nullptr || horizon_seconds <= 0.0f) {
    return path;
  }

  const int lead_ticks = apply_network_lead ? local_prediction_network_lead_ticks(entity) : 0;
  const float lead_time = local_prediction_ticks_to_time(lead_ticks);
  const int step_count = local_prediction_path_tick_count(horizon_seconds);

  Player* const entity_as_player = entity->get_class_id() == class_id::PLAYER ? static_cast<Player*>(entity) : nullptr;

  const bool can_use_engine_movement_sim =
    use_movement_sim &&
    entity_as_player != nullptr &&
    prediction != nullptr &&
    game_movement != nullptr &&
    move_helper != nullptr &&
    global_vars != nullptr;
  if (can_use_engine_movement_sim) {
    local_prediction_simulate_player_path(entity_as_player, horizon_seconds, &path, lead_ticks);
    return path;
  }

  Vec3 origin = entity->get_origin();
  Vec3 velocity = local_prediction_estimate_entity_velocity(entity);
  path.time_step = TICK_INTERVAL;
  path.start_time = lead_time;
  path.positions.reserve(static_cast<size_t>(step_count) + 1);

  origin += velocity * lead_time;
  path.positions.push_back(origin);
  for (int step = 1; step <= step_count; ++step) {
    const float time = std::min(horizon_seconds, static_cast<float>(step) * static_cast<float>(TICK_INTERVAL));
    path.positions.push_back(origin + (velocity * time));
  }
  path.final_velocity = velocity;
  path.valid = path.positions.size() > 1;
  return path;
}

inline Vec3 local_prediction_predict_entity_origin(Entity* entity, float horizon_seconds) {
  if (entity == nullptr) return Vec3{};
  LocalPredictionEntityPath path = local_prediction_predict_entity_path(entity, horizon_seconds);
  if (!path.valid || path.positions.empty()) {
    return entity->get_origin();
  }

  size_t index = std::min(
    path.positions.size() - 1,
    static_cast<size_t>(std::ceil(horizon_seconds / std::max(path.time_step, 0.0001f))));
  return path.positions[index];
}

inline Vec3 local_prediction_path_position_at_time(const LocalPredictionEntityPath& path, float time) {
  if (!path.valid || path.positions.empty()) {
    return Vec3{};
  }

  const float local_time = time - path.start_time;
  if (local_time <= 0.0f || path.positions.size() == 1) {
    return path.positions.front();
  }

  const float step = std::max(path.time_step, 0.0001f);
  const float exact_index = local_time / step;
  const size_t lower_index = std::min(
    static_cast<size_t>(std::floor(exact_index)),
    path.positions.size() - 1);
  const size_t upper_index = std::min(lower_index + 1, path.positions.size() - 1);
  const float fraction = std::clamp(exact_index - static_cast<float>(lower_index), 0.0f, 1.0f);

  return path.positions[lower_index] + ((path.positions[upper_index] - path.positions[lower_index]) * fraction);
}

namespace move_sim {

using path = LocalPredictionEntityPath;
using result = LocalPredictionResult;
using request = LocalPredictionRequest;
using tick = LocalPredictionTick;
using tick_result = LocalPredictionTickResult;

inline path predict_entity_path(Entity* entity,
  float horizon_seconds,
  bool use_movement_sim = true,
  bool apply_network_lead = false) {
  return local_prediction_predict_entity_path(entity, horizon_seconds, use_movement_sim, apply_network_lead);
}

inline bool simulate_player_path(Player* player, float horizon_seconds, path* out, int lead_ticks = 0) {
  return local_prediction_simulate_player_path(player, horizon_seconds, out, lead_ticks);
}

inline Vec3 predict_entity_origin(Entity* entity, float horizon_seconds) {
  return local_prediction_predict_entity_origin(entity, horizon_seconds);
}

inline Vec3 path_position_at_time(const path& predicted_path, float time) {
  return local_prediction_path_position_at_time(predicted_path, time);
}

}

#endif
