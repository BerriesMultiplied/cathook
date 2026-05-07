/*
/^-----^\   data: 2026-05-07
V  o o  V  file: src/games/tf2/sdk/interfaces/steam_user.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef STEAM_USER_HPP
#define STEAM_USER_HPP

#include <cstdint>

class steam_user
{
public:
  std::uint64_t get_steam_id()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto get_steam_id_fn = reinterpret_cast<std::uint64_t (*)(void*)>(vtable[2]);
    return get_steam_id_fn(this);
  }
};

inline static steam_user* steam_user_interface;

#endif
