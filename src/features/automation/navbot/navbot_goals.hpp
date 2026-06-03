/*
/^-----^\   data: 2026-04-05
V  o o  V  file: src/features/automation/navbot/navbot_goals.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef NAVBOT_GOALS_HPP
#define NAVBOT_GOALS_HPP

#include <string>
#include <unordered_map>

#include "features/automation/navbot/navbot_mesh.hpp"
#include "features/automation/navbot/navbot_types.hpp"

class Player;

namespace navbot
{

class navbot_goals
{
public:
  [[nodiscard]] navbot_goal_state select_goal(const navbot_mesh& mesh, Player* localplayer, float current_time);
  void mark_destination_unreachable(nav_area_id area_id, float current_time, float cooldown_seconds);
  void mark_roam_destination_cooldown(const navbot_mesh& mesh, nav_area_id area_id, float current_time);
  void prune_unreachable_destinations(float current_time);
  void clear_unreachable_destinations();

  [[nodiscard]] bool find_closest_payload_cart(const Vec3& local_origin, tf_team local_team, float max_distance, Vec3& out_origin, bool& out_is_our_team) const;

private:
  struct cached_flag_home
  {
    bool valid = false;
    Vec3 origin{};
  };

  void reset_flag_home_cache();
  void update_flag_home_cache(tf_team team, const Vec3& origin);
  [[nodiscard]] cached_flag_home flag_home_for_team(tf_team team) const;
  [[nodiscard]] goal_candidate choose_flag_goal(const navbot_mesh& mesh, Player* localplayer);
  [[nodiscard]] goal_candidate choose_roam_goal(const navbot_mesh& mesh, Player* localplayer, float current_time);
  [[nodiscard]] bool destination_recently_unreachable(nav_area_id area_id, float current_time) const;
  void reset_roam_state();
  void prune_roam_cooldowns(float current_time);
  void set_roam_area_cooldown(nav_area_id area_id, float expire_time);
  [[nodiscard]] bool roam_area_on_cooldown(nav_area_id area_id, float current_time) const;

  std::string cached_map_name_{};
  cached_flag_home red_flag_home_{};
  cached_flag_home blu_flag_home_{};
  nav_area_id last_roam_area_{};
  float next_roam_refresh_time_ = 0.0f;
  size_t roam_cursor_ = 0;
  std::unordered_map<uint32_t, float> unreachable_destinations_{};
  std::unordered_map<uint32_t, float> roam_cooldowns_{};
};

} // namespace navbot

#endif
