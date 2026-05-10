/*
/^-----^\   data: 2026-05-10
V  o o  V  file: src/features/combat/backtrack/backtrack.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef BACKTRACK_HPP
#define BACKTRACK_HPP

#include <array>
#include <climits>

#include "core/types.hpp"

class Player;
class Weapon;
struct aimbot_candidate;
struct user_cmd;

namespace backtrack
{

constexpr int max_entities = 65;
constexpr int max_records = 80;
constexpr int max_points = 10;

struct backtrack_bounds {
  bool valid = false;
  Vec3 mins{};
  Vec3 maxs{};
};

struct backtrack_point {
  bool valid = false;
  int bone = 0;
  int hitbox = -1;
  int priority = INT_MAX;
  Vec3 position{};
};

struct backtrack_record {
  bool valid = false;
  Player* player = nullptr;
  int ent_index = 0;
  float sim_time = 0.0f;
  float curtime = 0.0f;
  Vec3 origin{};
  backtrack_bounds bounds{};
  std::array<backtrack_point, max_points> points{};
  int point_count = 0;
};

struct player_records {
  int ent_index = 0;
  int record_count = 0;
  std::array<backtrack_record, max_records> records{};
};

void on_create_move(user_cmd* user_cmd);
void record_player(Player* player);
void clear();

[[nodiscard]] bool is_enabled();
[[nodiscard]] float fake_latency_seconds();
[[nodiscard]] float interpolation_time();
[[nodiscard]] const player_records* records_for_player(Player* player);
[[nodiscard]] aimbot_candidate find_hitscan_candidate(Player* localplayer,
  Weapon* weapon,
  Player* player,
  const Vec3& original_view_angles,
  bool preferred);

void draw_visualizer_imgui();

void install_net_channel_hook();
void restore_net_channel_hook();

} // namespace backtrack

#endif
