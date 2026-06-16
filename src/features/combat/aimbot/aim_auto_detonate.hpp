#ifndef AIM_AUTO_DETONATE_HPP
#define AIM_AUTO_DETONATE_HPP

#include <cstdint>
#include <vector>

#include "aim_utils.hpp"
#include "proj_aim/proj_aim_trace.hpp"

namespace aim_auto_detonate {

inline bool projectile_pipebomb_is_sticky(Entity* entity) {
  if (entity == nullptr || entity->get_class_id() != class_id::PILL_OR_STICKY) {
    return false;
  }

  static int type_offset = tf2_netvars::find_offset("CTFGrenadePipebombProjectile", {"m_iType"});
  if (type_offset <= 0) {
    return false;
  }

  const int type = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(entity) + type_offset);
  return type == 1 || type == 2;
}

inline float projectile_pipebomb_creation_time(Entity* entity) {
  if (entity == nullptr || entity->get_class_id() != class_id::PILL_OR_STICKY) {
    return 0.0f;
  }

  static int creation_time_offset = []() -> int {
    const int type_offset = tf2_netvars::find_offset("CTFGrenadePipebombProjectile", {"m_iType"});
    if (type_offset <= 0) {
      return 0;
    }
    return type_offset + 4;
  }();
  if (creation_time_offset <= 0) {
    return 0.0f;
  }

  return *reinterpret_cast<float*>(reinterpret_cast<std::uintptr_t>(entity) + creation_time_offset);
}

inline bool projectile_supported_weapon(Weapon* weapon) {
  if (weapon == nullptr) {
    return false;
  }

  return proj_aim_is_stickybomb_launcher(weapon) || weapon->get_def_id() == Pyro_s_TheDetonator;
}

inline Entity* projectile_launcher_entity(Entity* projectile) {
  if (projectile == nullptr) {
    return nullptr;
  }

  const int class_id_value = projectile->get_class_id();
  if (class_id_value == class_id::PILL_OR_STICKY) {
    static int launcher_offset = tf2_netvars::find_offset("CTFGrenadePipebombProjectile", {"m_hLauncher"});
    if (launcher_offset > 0) {
      const int handle = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(projectile) + launcher_offset);
      if (handle != 0 && entity_list != nullptr) {
        return entity_list->entity_from_handle(handle);
      }
    }
  } else if (class_id_value == class_id::FLARE) {
    static int launcher_offset = tf2_netvars::find_offset("CTFBaseRocket", {"m_hLauncher"});
    if (launcher_offset > 0) {
      const int handle = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(projectile) + launcher_offset);
      if (handle != 0 && entity_list != nullptr) {
        return entity_list->entity_from_handle(handle);
      }
    }
  }

  return projectile->get_owner_entity();
}

inline bool projectile_owned_by_local(Player* localplayer, Entity* projectile) {
  Entity* launcher = projectile_launcher_entity(projectile);
  if (localplayer == nullptr || projectile == nullptr) {
    return false;
  }

  if (launcher == localplayer) {
    return true;
  }

  Entity* owner = projectile->get_owner_entity();
  return owner == localplayer || (launcher != nullptr && launcher->get_owner_entity() == localplayer);
}

inline bool target_can_detonate(Player* localplayer,
  Weapon* weapon,
  Entity* projectile,
  Entity* target,
  float radius,
  float age_seconds) {
  if (localplayer == nullptr || weapon == nullptr || projectile == nullptr || target == nullptr || radius <= 0.0f) {
    return false;
  }

  const Vec3 explosion_origin = projectile->get_origin();
  if (target->get_class_id() == class_id::PLAYER) {
    Player* target_player = static_cast<Player*>(target);
    const uint32_t hitbox_mask = proj_aim_effective_hitbox_mask(localplayer, weapon, target_player);
    const float effective_radius = proj_aim_is_stickybomb_launcher(weapon)
      ? proj_aim_splash_radius_for_target(weapon, radius, age_seconds, !target_player->is_on_ground())
      : radius;
    return effective_radius > 0.0f &&
      proj_aim_explosion_can_damage(localplayer, target_player, explosion_origin, effective_radius, hitbox_mask);
  }

  if (!target->is_building() || !aimbot_entity_is_enemy_owned(localplayer, target)) {
    return false;
  }

  const Vec3 target_origin = aimbot_entity_target_position(target);
  if (distance_3d(explosion_origin, target_origin) > radius) {
    return false;
  }

  trace_t trace{};
  if (!projectile_trace_ray(
      explosion_origin,
      target_origin,
      nullptr,
      nullptr,
      projectile_trace_contract::radius_damage,
      localplayer,
      static_cast<int>(localplayer->get_team()),
      &trace,
      target)) {
    return false;
  }

  return trace.entity == target || projectile_trace_clear(trace, 0.97f);
}

inline bool apply(user_cmd* user_cmd, Player* localplayer, Weapon* weapon) {
  if (user_cmd == nullptr || localplayer == nullptr || weapon == nullptr || !config.aimbot.auto_detonate || !projectile_supported_weapon(weapon)) {
    return false;
  }

  const float base_radius = proj_aim_splash_radius_for_weapon(weapon);
  if (base_radius <= 0.0f) {
    return false;
  }

  const float current_time = global_vars != nullptr ? global_vars->curtime : 0.0f;
  const bool flare_weapon = weapon->get_def_id() == Pyro_s_TheDetonator;
  const std::vector<Entity*>& stickies = entity_cache_entities(class_id::PILL_OR_STICKY);
  const std::vector<Entity*>& flares = entity_cache_entities(class_id::FLARE);
  const std::vector<Entity*>& sentries = entity_cache_entities(class_id::SENTRY);
  const std::vector<Entity*>& dispensers = entity_cache_entities(class_id::DISPENSER);
  const std::vector<Entity*>& teleporters = entity_cache_entities(class_id::TELEPORTER);

  for (const entity_cache_player_entry& entry : entity_cache_players()) {
    Player* target_player = entry.player;
    if (target_player == nullptr || aimbot_player_skip_reason_for(localplayer, target_player) != aimbot_player_skip_reason::none) {
      continue;
    }

    const float lead_time = local_prediction_ticks_to_time(local_prediction_network_lead_ticks(target_player));
    const Vec3 target_origin = entry.origin + (entry.velocity * lead_time);

    if (flare_weapon) {
      for (Entity* flare_entity : flares) {
        if (!projectile_owned_by_local(localplayer, flare_entity)) {
          continue;
        }

        if (distance_3d(flare_entity->get_origin(), target_origin) > base_radius) {
          continue;
        }

        if (proj_aim_explosion_can_damage(localplayer, target_player, flare_entity->get_origin(), base_radius, proj_aim_effective_hitbox_mask(localplayer, weapon, target_player))) {
          user_cmd->buttons |= IN_ATTACK2;
          return true;
        }
      }
      continue;
    }

    for (Entity* projectile : stickies) {
      if (!projectile_owned_by_local(localplayer, projectile) || !projectile_pipebomb_is_sticky(projectile)) {
        continue;
      }

      const float creation_time = projectile_pipebomb_creation_time(projectile);
      if (creation_time <= 0.0f || current_time < creation_time) {
        continue;
      }

      const float age_seconds = current_time - creation_time;
      const float effective_radius = proj_aim_splash_radius_for_target(weapon, base_radius, age_seconds, !target_player->is_on_ground());
      if (effective_radius <= 0.0f || distance_3d(projectile->get_origin(), target_origin) > effective_radius) {
        continue;
      }

      if (proj_aim_explosion_can_damage(localplayer, target_player, projectile->get_origin(), effective_radius, proj_aim_effective_hitbox_mask(localplayer, weapon, target_player))) {
        user_cmd->buttons |= IN_ATTACK2;
        return true;
      }
    }
  }

  for (Entity* target_entity : sentries) {
    if (target_entity == nullptr || !aimbot_entity_is_enemy_owned(localplayer, target_entity)) {
      continue;
    }

    const Vec3 target_origin = aimbot_entity_target_position(target_entity);
    for (Entity* projectile : stickies) {
      if (!projectile_owned_by_local(localplayer, projectile) || !projectile_pipebomb_is_sticky(projectile)) {
        continue;
      }

      const float creation_time = projectile_pipebomb_creation_time(projectile);
      if (creation_time <= 0.0f || current_time < creation_time) {
        continue;
      }

      const float age_seconds = current_time - creation_time;
      const float effective_radius = proj_aim_splash_radius_for_target(weapon, base_radius, age_seconds, false);
      if (effective_radius <= 0.0f || distance_3d(projectile->get_origin(), target_origin) > effective_radius) {
        continue;
      }

      user_cmd->buttons |= IN_ATTACK2;
      return true;
    }
  }

  for (Entity* target_entity : dispensers) {
    if (target_entity == nullptr || !aimbot_entity_is_enemy_owned(localplayer, target_entity)) {
      continue;
    }

    const Vec3 target_origin = aimbot_entity_target_position(target_entity);
    for (Entity* projectile : stickies) {
      if (!projectile_owned_by_local(localplayer, projectile) || !projectile_pipebomb_is_sticky(projectile)) {
        continue;
      }

      const float creation_time = projectile_pipebomb_creation_time(projectile);
      if (creation_time <= 0.0f || current_time < creation_time) {
        continue;
      }

      const float age_seconds = current_time - creation_time;
      const float effective_radius = proj_aim_splash_radius_for_target(weapon, base_radius, age_seconds, false);
      if (effective_radius <= 0.0f || distance_3d(projectile->get_origin(), target_origin) > effective_radius) {
        continue;
      }

      user_cmd->buttons |= IN_ATTACK2;
      return true;
    }
  }

  for (Entity* target_entity : teleporters) {
    if (target_entity == nullptr || !aimbot_entity_is_enemy_owned(localplayer, target_entity)) {
      continue;
    }

    const Vec3 target_origin = aimbot_entity_target_position(target_entity);
    for (Entity* projectile : stickies) {
      if (!projectile_owned_by_local(localplayer, projectile) || !projectile_pipebomb_is_sticky(projectile)) {
        continue;
      }

      const float creation_time = projectile_pipebomb_creation_time(projectile);
      if (creation_time <= 0.0f || current_time < creation_time) {
        continue;
      }

      const float age_seconds = current_time - creation_time;
      const float effective_radius = proj_aim_splash_radius_for_target(weapon, base_radius, age_seconds, false);
      if (effective_radius <= 0.0f || distance_3d(projectile->get_origin(), target_origin) > effective_radius) {
        continue;
      }

      user_cmd->buttons |= IN_ATTACK2;
      return true;
    }
  }

  return false;
}

}

#endif
