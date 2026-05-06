/*
/^-----^\   data: 2026-05-06
V  o o  V  file: src/games/tf2/sdk/interfaces/steam_networking_utils.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef STEAM_NETWORKING_UTILS_HPP
#define STEAM_NETWORKING_UTILS_HPP

#include <cstdint>

using steam_networking_pop_id = std::uint32_t;

class steam_networking_utils
{
public:
  bool check_ping_data_up_to_date(float max_age_seconds)
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto call_check_ping_data_up_to_date = reinterpret_cast<bool (*)(void*, float)>(vtable[7]);
    return call_check_ping_data_up_to_date(this, max_age_seconds);
  }

  int get_ping_to_data_center(steam_networking_pop_id pop_id, steam_networking_pop_id* via_relay_pop)
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto call_get_ping_to_data_center =
      reinterpret_cast<int (*)(void*, steam_networking_pop_id, steam_networking_pop_id*)>(vtable[8]);
    return call_get_ping_to_data_center(this, pop_id, via_relay_pop);
  }

  int get_direct_ping_to_pop(steam_networking_pop_id pop_id)
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto call_get_direct_ping_to_pop = reinterpret_cast<int (*)(void*, steam_networking_pop_id)>(vtable[9]);
    return call_get_direct_ping_to_pop(this, pop_id);
  }
};

inline steam_networking_utils* steam_networking_utils_interface = nullptr;

#endif
