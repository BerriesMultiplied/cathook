/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/calc_is_attack_critical.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/combat/random_crits/random_crits.hpp"
#include "games/tf2/sdk/entities/player.hpp"
#include "games/tf2/sdk/interfaces/global_vars.hpp"

bool (*calc_is_attack_critical_original)(void*);

bool calc_is_attack_critical_hook(void* me)
{
  auto* weapon = reinterpret_cast<Weapon*>(me);
  auto* owner = weapon != nullptr ? reinterpret_cast<Player*>(weapon->to_entity()->get_owner_entity()) : nullptr;
  auto* cmd = owner != nullptr ? owner->get_current_cmd() : nullptr;

  const int old_framecount = global_vars != nullptr ? global_vars->framecount : 0;
  if (global_vars != nullptr && cmd != nullptr) {
    global_vars->framecount = cmd->command_number;
  }

  const bool result = calc_is_attack_critical_original(me);

  if (global_vars != nullptr && cmd != nullptr) {
    global_vars->framecount = old_framecount;
  }

  if (random_crits::should_force_attack(weapon, cmd)) {
    weapon->current_attack_is_crit() = true;
    weapon->current_crit_is_random() = true;
    if (weapon->is_rapid_fire() && global_vars != nullptr) {
      weapon->crit_time() = global_vars->curtime + 2.0f;
    }
    return true;
  }

  if (random_crits::should_skip_attack(weapon, cmd)) {
    weapon->current_attack_is_crit() = false;
    weapon->current_crit_is_random() = false;
    return false;
  }

  return result;
}
