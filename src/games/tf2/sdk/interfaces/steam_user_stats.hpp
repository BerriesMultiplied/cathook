/*
/^-----^\   data: 2026-05-05
V  o o  V  file: src/games/tf2/sdk/interfaces/steam_user_stats.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef STEAM_USER_STATS_HPP
#define STEAM_USER_STATS_HPP

class steam_user_stats
{
public:
  bool request_current_stats()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto request_current_stats_fn = reinterpret_cast<bool (*)(void*)>(vtable[0]);
    return request_current_stats_fn(this);
  }

  bool clear_achievement(const char* name)
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto clear_achievement_fn = reinterpret_cast<bool (*)(void*, const char*)>(vtable[8]);
    return clear_achievement_fn(this, name);
  }

  bool store_stats()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto store_stats_fn = reinterpret_cast<bool (*)(void*)>(vtable[10]);
    return store_stats_fn(this);
  }
};

inline static steam_user_stats* steam_user_stats_interface;

#endif
