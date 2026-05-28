#ifndef CATHOOK_IDENTIFY_HPP
#define CATHOOK_IDENTIFY_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace cathook::core::identify
{

void start();
void stop();

void tick();

[[nodiscard]] std::string player_hash(std::uint32_t friends_id, std::string_view name);
[[nodiscard]] bool is_peer(std::uint32_t account_id, std::string_view name);
[[nodiscard]] bool is_peer_hash(std::string_view hash);
[[nodiscard]] bool is_betrayed(std::uint32_t account_id);

void on_player_death(int attacker_user_id);
void clear_betrayals();

} // namespace cathook::core::identify

#endif
