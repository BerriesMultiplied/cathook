#ifndef AIM_AUTO_SHOOT_HPP
#define AIM_AUTO_SHOOT_HPP

#include "aim_state.hpp"
#include "aim_utils.hpp"

#include "games/tf2/sdk/interfaces/attribute_manager.hpp"

namespace aim_auto_shoot {

struct result {
  bool requested = false;
  bool primary_attack = false;
  bool release_attack = false;
};

inline bool projectile_is_charge_weapon(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  switch (weapon->get_weapon_id()) {
  case TF_WEAPON_COMPOUND_BOW:
  case TF_WEAPON_PIPEBOMBLAUNCHER:
  case TF_WEAPON_CANNON:
    return true;
  default:
    return false;
  }
}

inline bool projectile_charge_started(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  switch (weapon->get_weapon_id()) {
  case TF_WEAPON_COMPOUND_BOW:
  case TF_WEAPON_PIPEBOMBLAUNCHER:
    return weapon->get_charge_begin_time() > 0.0f;
  case TF_WEAPON_CANNON:
    return weapon->get_detonate_time() > 0.0f;
  default:
    return false;
  }
}

inline bool weapon_can_overload(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  if (attribute_manager != nullptr &&
      attribute_manager->attrib_hook_value(0, "can_overload", weapon->to_entity()) != 0) {
    return true;
  }

  return weapon->get_def_id() == Soldier_m_TheBeggarsBazooka;
}

inline bool weapon_has_primary_ammo(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  if (aimbot_is_melee_weapon(weapon) ||
      projectile_charge_started(weapon) ||
      (weapon_can_overload(weapon) && weapon->get_clip1() > 0)) {
    return true;
  }

  const int clip = weapon->get_clip1();
  return clip != 0;
}

inline bool weapon_should_clear_secondary(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  switch (weapon->get_weapon_id()) {
  case TF_WEAPON_ROCKETLAUNCHER:
  case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
  case TF_WEAPON_GRENADELAUNCHER:
  case TF_WEAPON_FLAREGUN:
  case TF_WEAPON_CROSSBOW:
  case TF_WEAPON_RAYGUN:
  case TF_WEAPON_PARTICLE_CANNON:
  case TF_WEAPON_DRG_POMSON:
  case TF_WEAPON_SYRINGEGUN_MEDIC:
    return true;
  default:
    return false;
  }
}

inline bool weapon_has_release_shot_ready(Weapon* weapon, bool projectile_solution, bool hitscan_solution) {
  if (weapon == nullptr) {
    return false;
  }

  if (projectile_solution) {
    if (projectile_is_charge_weapon(weapon) && projectile_charge_started(weapon)) {
      return true;
    }
    if (weapon_can_overload(weapon) && weapon->get_clip1() > 0) {
      return true;
    }
  }

  return hitscan_solution &&
    weapon->get_weapon_id() == TF_WEAPON_SNIPERRIFLE_CLASSIC &&
    weapon->get_charged_damage() > 0.0f;
}

inline void reset_state(Weapon* weapon) {
  if (aim_state::auto_shoot.weapon == weapon) {
    return;
  }

  aim_state::auto_shoot.weapon = weapon;
  aim_state::auto_shoot.holding_charge = false;
  aim_state::auto_shoot.holding_overload = false;
}

inline bool hitscan_cached_primary_ready(Player* localplayer, Weapon* weapon) {
  static Weapon* last_weapon = nullptr;
  static float cached_next_primary_attack = 0.0f;
  static float last_fire_time = 0.0f;

  if (localplayer == nullptr || weapon == nullptr || aimbot_is_projectile_weapon(weapon) || aimbot_is_melee_weapon(weapon)) {
    return false;
  }

  const float weapon_last_fire_time = weapon->get_last_attack();
  if (weapon != last_weapon || weapon_last_fire_time != last_fire_time) {
    cached_next_primary_attack = weapon->get_next_primary_attack();
    last_fire_time = weapon_last_fire_time;
    last_weapon = weapon;
  }

  const float tick_interval = global_vars != nullptr && global_vars->interval_per_tick > 0.0f
    ? global_vars->interval_per_tick
    : static_cast<float>(TICK_INTERVAL);
  const float server_time = static_cast<float>(localplayer->get_tickbase()) * tick_interval;
  return cached_next_primary_attack <= server_time;
}

inline bool weapon_can_attack_or_release(Player* localplayer, Weapon* weapon) {
  if (localplayer == nullptr || weapon == nullptr) {
    return false;
  }

  if (weapon->can_primary_attack()) {
    return true;
  }
  if (hitscan_cached_primary_ready(localplayer, weapon)) {
    return true;
  }
  if (weapon_has_release_shot_ready(weapon, aimbot_is_projectile_weapon(weapon), !aimbot_is_projectile_weapon(weapon) && !aimbot_is_melee_weapon(weapon))) {
    return true;
  }

  return aimbot_is_projectile_weapon(weapon) &&
    ((projectile_is_charge_weapon(weapon) && projectile_charge_started(weapon)) ||
      (weapon_can_overload(weapon) && aim_state::auto_shoot.holding_overload));
}

inline result apply(user_cmd* user_cmd, Weapon* weapon, bool projectile_solution, bool hitscan_solution, bool melee_solution) {
  result r{};
  if (user_cmd == nullptr || weapon == nullptr) {
    return r;
  }

  reset_state(weapon);

  if (!weapon_has_primary_ammo(weapon)) {
    return r;
  }

  if (weapon_has_release_shot_ready(weapon, projectile_solution, hitscan_solution)) {
    user_cmd->buttons &= ~IN_ATTACK;
    r.requested = true;
    r.release_attack = true;
    aim_state::auto_shoot.holding_charge = false;
    aim_state::auto_shoot.holding_overload = false;
    return r;
  }

  if (projectile_solution && weapon_can_overload(weapon)) {
    user_cmd->buttons |= IN_ATTACK;
    r.requested = true;
    r.primary_attack = true;
    aim_state::auto_shoot.holding_overload = true;
    return r;
  }

  if (projectile_solution && projectile_is_charge_weapon(weapon)) {
    user_cmd->buttons |= IN_ATTACK;
    r.requested = true;
    r.primary_attack = true;
    aim_state::auto_shoot.holding_charge = true;
    return r;
  }

  (void)melee_solution;

  user_cmd->buttons |= IN_ATTACK;
  if (projectile_solution && weapon_should_clear_secondary(weapon)) {
    user_cmd->buttons &= ~IN_ATTACK2;
  }

  r.requested = true;
  r.primary_attack = true;
  return r;
}

inline void hold_charge_if_needed(user_cmd* user_cmd, Weapon* weapon) {
  if (user_cmd == nullptr || weapon == nullptr || !config.aimbot.auto_shoot) {
    return;
  }

  reset_state(weapon);

  if (projectile_is_charge_weapon(weapon) && projectile_charge_started(weapon)) {
    user_cmd->buttons |= IN_ATTACK;
    aim_state::auto_shoot.holding_charge = true;
    return;
  }

  if (weapon_can_overload(weapon) && aim_state::auto_shoot.holding_overload) {
    user_cmd->buttons |= IN_ATTACK;
  }
}

}

#endif
