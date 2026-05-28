#ifndef CATHOOK_IDENTIFY_HPP
#define CATHOOK_IDENTIFY_HPP

#include <cstdint>
#include <string_view>

namespace cathook::core::identify
{

// called once
void start();
void stop();

void tick();

// uh oh stinky nodiscord p ew
[[nodiscard]] bool is_peer(std::uint32_t account_id, std::string_view name);

// betraye;ls
void on_player_death(int attacker_user_id);
void clear_betrayals();

} // namespace cathook::core::identify

#endif
