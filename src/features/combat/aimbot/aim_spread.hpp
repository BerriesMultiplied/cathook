#ifndef AIM_SPREAD_HPP
#define AIM_SPREAD_HPP

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>

#include "MD5/MD5.hpp"
#include "core/shared/sigs.hpp"
#include "libsigscan/libsigscan.h"

#include "aim_utils.hpp"
#include "hitscan_aim.hpp"

#include "features/combat/aimbot/proj_aim.hpp"
#include "features/movement/local_prediction/local_prediction.hpp"

#include "games/tf2/sdk/interfaces/convar_system.hpp"

namespace aim_spread {

using random_seed_fn = void (*)(int);
using random_float_fn = float (*)(float, float);
using bullet_spread_fn = float (*)(void*);

inline random_seed_fn random_seed = nullptr;
inline random_float_fn random_float = nullptr;
inline bullet_spread_fn get_bullet_spread = nullptr;
inline bool random_initialized = false;
inline bool bullet_spread_initialized = false;
inline bool bullet_spread_signature_found = false;

inline bool init_random() {
  if (random_initialized) {
    return random_seed != nullptr && random_float != nullptr;
  }

  random_initialized = true;
  random_seed = reinterpret_cast<random_seed_fn>(dlsym(RTLD_DEFAULT, "RandomSeed"));
  random_float = reinterpret_cast<random_float_fn>(dlsym(RTLD_DEFAULT, "RandomFloat"));
  return random_seed != nullptr && random_float != nullptr;
}

inline bool init_bullet_spread() {
  if (bullet_spread_initialized) {
    return get_bullet_spread != nullptr;
  }

  bullet_spread_initialized = true;
  get_bullet_spread = reinterpret_cast<bullet_spread_fn>(
    sigscan_module("client.so", sigs::tf_weapon_base_gun_get_bullet_spread));
  bullet_spread_signature_found = get_bullet_spread != nullptr;
  return get_bullet_spread != nullptr;
}

inline float weapon_hitscan_spread(Weapon* weapon) {
  if (weapon == nullptr || aimbot_is_projectile_weapon(weapon) || aimbot_is_melee_weapon(weapon)) {
    return 0.0f;
  }

  if (init_bullet_spread()) {
    const float spread = get_bullet_spread(weapon);
    if (std::isfinite(spread) && spread > 0.0f) {
      return std::clamp(spread, 0.0f, 1.0f);
    }
  }

  return weapon->get_hitscan_spread();
}

inline bool fixed_weapon_spreads_enabled() {
  if (convar_system == nullptr) {
    return false;
  }

  static Convar* fixed_weapon_spreads = convar_system->find_var("tf_use_fixed_weaponspreads");
  return fixed_weapon_spreads != nullptr && fixed_weapon_spreads->get_int() != 0;
}

inline float spread_rand_float_valve(int* stream_seed, float lo, float hi) {
  if (stream_seed == nullptr) {
    return lo;
  }

  *stream_seed = (*stream_seed) * 214013 + 2531011;
  const int r = (*stream_seed >> 16) & 0x7fff;
  const float t = static_cast<float>(r) * (1.0f / 32768.0f);
  return lo + (hi - lo) * t;
}

inline bool hitscan_spread_offset(user_cmd* user_cmd, int pellet_index, float spread, Vec3* offset_out) {
  if (offset_out == nullptr) {
    return false;
  }

  *offset_out = {};
  if (user_cmd == nullptr || user_cmd->command_number <= 0 || spread <= 0.0f) {
    return true;
  }

  const int base_seed =
    (MD5_PseudoRandom(static_cast<unsigned int>(user_cmd->command_number)) & INT_MAX) + std::max(0, pellet_index);
  int stream = base_seed;
  constexpr float spread_scale = 0.5f;
  offset_out->x = (spread_rand_float_valve(&stream, -spread_scale, spread_scale) +
    spread_rand_float_valve(&stream, -spread_scale, spread_scale)) * spread;
  offset_out->y = (spread_rand_float_valve(&stream, -spread_scale, spread_scale) +
    spread_rand_float_valve(&stream, -spread_scale, spread_scale)) * spread;
  return true;
}

inline Vec3 compensate_hitscan_spread(Player* localplayer, const Vec3& desired_command_angles, const Vec3& spread_offset) {
  if (localplayer == nullptr) {
    return desired_command_angles;
  }

  const Vec3 desired_bullet_angles = hitscan_aim_bullet_angles(localplayer, desired_command_angles);
  Vec3 adjusted_bullet_angles = desired_bullet_angles;
  for (int iteration = 0; iteration < 2; ++iteration) {
    Vec3 forward{};
    Vec3 right{};
    Vec3 up{};
    angle_vectors(adjusted_bullet_angles, &forward, &right, &up);
    Vec3 spread_direction = local_prediction_normalize(forward + (right * spread_offset.x) + (up * spread_offset.y));
    if (!aimbot_vec3_is_finite(spread_direction)) {
      break;
    }

    const Vec3 spread_angles = local_prediction_direction_to_angles(spread_direction);
    const Vec3 error = aimbot_normalize_angle_delta(spread_angles, desired_bullet_angles);
    adjusted_bullet_angles = aimbot_clamp_angles(adjusted_bullet_angles - error);
  }

  return aimbot_clamp_angles(hitscan_aim_command_angles(localplayer, adjusted_bullet_angles));
}

struct hitscan_fire_solution {
  bool ready = false;
  bool spread_compensated = false;
  bool spread_signature = false;
  bool spread_fixed = false;
  bool seed_missing = false;
  bool hit_wrong_hitbox = false;
  int pellet_count = 0;
  int pellet_index = -1;
  int trace_hitbox = -1;
  int trace_entity_index = -1;
  float spread = 0.0f;
  Vec3 command_angles{};
};

inline hitscan_fire_solution prepare_hitscan_fire_solution(Player* localplayer,
  Weapon* weapon,
  user_cmd* user_cmd,
  const aimbot_candidate& candidate,
  const Vec3& command_view_angles) {
  hitscan_fire_solution solution{};
  solution.command_angles = command_view_angles;
  solution.spread_signature = bullet_spread_signature_found;
  if (localplayer == nullptr || weapon == nullptr || user_cmd == nullptr || candidate.entity == nullptr) {
    return solution;
  }

  const float spread = weapon_hitscan_spread(weapon);
  const bool use_spread = config.aimbot.spread_compensation && spread > 0.00001f;
  const int pellet_count = use_spread ? std::max(1, weapon->get_bullets_per_shot()) : 1;
  solution.spread = spread;
  solution.pellet_count = pellet_count;
  solution.spread_signature = bullet_spread_signature_found;
  solution.spread_fixed = use_spread && fixed_weapon_spreads_enabled();

  for (int pellet_index = 0; pellet_index < pellet_count; ++pellet_index) {
    Vec3 spread_offset{};
    if (use_spread && !hitscan_spread_offset(user_cmd, pellet_index, spread, &spread_offset)) {
      solution.seed_missing = true;
      break;
    }

    const Vec3 trace_angles = use_spread
      ? compensate_hitscan_spread(localplayer, command_view_angles, spread_offset)
      : command_view_angles;

    hitscan_aim_trace_result trace_result{};
    if (!hitscan_aim_trace_candidate(localplayer, candidate, trace_angles, spread_offset, use_spread, &trace_result)) {
      if (trace_result.entity == candidate.entity &&
          candidate.hitbox >= 0 &&
          trace_result.hitbox >= 0 &&
          trace_result.hitbox != candidate.hitbox) {
        solution.hit_wrong_hitbox = true;
        solution.trace_hitbox = trace_result.hitbox;
        solution.trace_entity_index = trace_result.entity->get_index();
      }
      continue;
    }

    solution.ready = true;
    solution.command_angles = trace_angles;
    solution.spread_compensated = use_spread;
    solution.pellet_index = use_spread ? pellet_index : -1;
    solution.trace_hitbox = trace_result.hitbox;
    solution.trace_entity_index = trace_result.entity != nullptr ? trace_result.entity->get_index() : -1;
    return solution;
  }

  return solution;
}

inline bool hitscan_candidate_ready_for_selection(Player* localplayer, Weapon* weapon, user_cmd* user_cmd, const aimbot_candidate& candidate) {
  if (localplayer == nullptr ||
      weapon == nullptr ||
      user_cmd == nullptr ||
      candidate.entity == nullptr ||
      aimbot_is_projectile_weapon(weapon) ||
      aimbot_is_melee_weapon(weapon)) {
    return false;
  }

  const Vec3 command_angles = candidate.player != nullptr
    ? candidate.command_angles
    : candidate.aim_angles - localplayer->get_punch_angles();
  return prepare_hitscan_fire_solution(localplayer, weapon, user_cmd, candidate, command_angles).ready;
}

inline uint32_t crc32_process_byte(uint32_t crc, uint8_t value) {
  crc ^= value;
  for (int bit = 0; bit < 8; ++bit) {
    const uint32_t mask = 0U - (crc & 1U);
    crc = (crc >> 1) ^ (0xEDB88320U & mask);
  }
  return crc;
}

inline uint32_t crc32_process_buffer(uint32_t crc, const void* data, int size) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  for (int index = 0; index < size; ++index) {
    crc = crc32_process_byte(crc, bytes[index]);
  }
  return crc;
}

inline int seed_file_line_hash(int seed, const char* name, int additional_seed) {
  uint32_t crc = 0xFFFFFFFFU;
  crc = crc32_process_buffer(crc, &seed, sizeof(seed));
  crc = crc32_process_buffer(crc, &additional_seed, sizeof(additional_seed));
  crc = crc32_process_buffer(crc, name, static_cast<int>(std::strlen(name)));
  return static_cast<int>(~crc);
}

inline bool projectile_is_syringe_weapon(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  switch (weapon->get_def_id()) {
  case Medic_m_SyringeGun:
  case Medic_m_SyringeGunR:
  case Medic_m_TheBlutsauger:
  case Medic_m_TheOverdose:
    return true;
  default:
    return false;
  }
}

inline bool projectile_has_pipe_random_velocity(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  switch (weapon->get_def_id()) {
  case Demoman_m_GrenadeLauncher:
  case Demoman_m_GrenadeLauncherR:
  case Demoman_m_TheLochnLoad:
  case Demoman_m_TheLooseCannon:
  case Demoman_m_FestiveGrenadeLauncher:
  case Demoman_m_TheIronBomber:
  case Demoman_s_StickybombLauncher:
  case Demoman_s_StickybombLauncherR:
  case Demoman_s_FestiveStickybombLauncher:
  case Demoman_s_TheScottishResistance:
  case Demoman_s_TheQuickiebombLauncher:
    return true;
  default:
    return false;
  }
}

inline Vec3 projectile_random_angle_offset(Player* localplayer, Weapon* weapon, user_cmd* user_cmd, const Vec3& view_angles) {
  if (localplayer == nullptr || weapon == nullptr || user_cmd == nullptr || user_cmd->command_number <= 0) {
    return Vec3{};
  }

  if (!projectile_is_syringe_weapon(weapon) && !projectile_has_pipe_random_velocity(weapon)) {
    return Vec3{};
  }

  if (!init_random()) {
    return Vec3{};
  }

  const int command_seed = MD5_PseudoRandom(static_cast<unsigned int>(user_cmd->command_number)) & INT_MAX;
  random_seed(seed_file_line_hash(command_seed, "SelectWeightedSequence", 0));
  for (int index = 0; index < 6; ++index) {
    random_float(0.0f, 1.0f);
  }

  Vec3 angle_offset{};
  if (projectile_is_syringe_weapon(weapon)) {
    angle_offset.x += random_float(-1.5f, 1.5f);
    angle_offset.y += random_float(-1.5f, 1.5f);
    return angle_offset;
  }

  projectile_sim_profile profile = projectile_sim_profile_for_weapon(localplayer, weapon);
  if (!profile.valid || profile.params.speed <= 0.0f) {
    return Vec3{};
  }

  Vec3 forward{};
  Vec3 right{};
  Vec3 up{};
  angle_vectors(view_angles, &forward, &right, &up);

  const Vec3 velocity = (forward * profile.params.speed) + (up * profile.initial_lift);
  const Vec3 velocity_random = velocity +
    (right * random_float(-10.0f, 10.0f)) +
    (up * random_float(-10.0f, 10.0f));
  if (local_prediction_vec3_is_zero(velocity) || local_prediction_vec3_is_zero(velocity_random)) {
    return Vec3{};
  }

  return aimbot_normalize_angle_delta(
    local_prediction_direction_to_angles(local_prediction_normalize(velocity_random)),
    local_prediction_direction_to_angles(local_prediction_normalize(velocity)));
}

inline Vec3 apply_projectile_random_compensation(Player* localplayer, Weapon* weapon, user_cmd* user_cmd, const Vec3& view_angles) {
  const Vec3 angle_offset = projectile_random_angle_offset(localplayer, weapon, user_cmd, view_angles);
  if (local_prediction_vec3_is_zero(angle_offset)) {
    return view_angles;
  }

  return aimbot_clamp_angles(view_angles - angle_offset);
}

}

#endif
