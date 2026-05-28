/*
/^-----^\   data: 2026-05-06
V  o o  V  file: src/core/player_manager.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef PLAYER_MANAGER_HPP
#define PLAYER_MANAGER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cathook::core::players
{

enum class player_state
{
  default_state = 0,
  friend_state,
  ignored,
  cheater,
  ipc,
  textmode,
  party,
  identified,
};

struct player_entry
{
  std::uint32_t account_id = 0;
  player_state state = player_state::default_state;
  std::string name{};
  bool runtime = false;
};

void initialize();
void shutdown();
void tick();

bool load();
bool save();
bool set_state(std::uint32_t account_id, player_state state, std::string_view name = {}, bool save_changes = true);
bool clear_state(std::uint32_t account_id, bool save_changes = true);

[[nodiscard]] player_state state_for(std::uint32_t account_id);
[[nodiscard]] bool is_friendly(std::uint32_t account_id);
[[nodiscard]] bool is_ignored(std::uint32_t account_id);
[[nodiscard]] bool is_prioritized(std::uint32_t account_id);
[[nodiscard]] bool is_cat(std::uint32_t account_id);
[[nodiscard]] bool state_is_cat(player_state state);
[[nodiscard]] const char* state_name(player_state state);
[[nodiscard]] std::optional<player_state> parse_state(std::string_view state_name);
[[nodiscard]] std::vector<player_entry> entries(bool include_runtime);
[[nodiscard]] std::uint32_t account_id_for_player_index(int player_index);

} // namespace cathook::core::players

#endif
