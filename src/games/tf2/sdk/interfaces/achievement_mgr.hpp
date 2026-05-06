/*
/^-----^\   data: 2026-05-05
V  o o  V  file: src/games/tf2/sdk/interfaces/achievement_mgr.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef ACHIEVEMENT_MGR_HPP
#define ACHIEVEMENT_MGR_HPP

class achievement
{
public:
  int get_achievement_id()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto get_achievement_id_fn = reinterpret_cast<int (*)(void*)>(vtable[0]);
    return get_achievement_id_fn(this);
  }

  const char* get_name()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto get_name_fn = reinterpret_cast<const char* (*)(void*)>(vtable[1]);
    return get_name_fn(this);
  }

  bool is_achieved()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto is_achieved_fn = reinterpret_cast<bool (*)(void*)>(vtable[5]);
    return is_achieved_fn(this);
  }
};

class achievement_manager
{
public:
  achievement* get_achievement_by_index(int index)
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto get_achievement_by_index_fn = reinterpret_cast<achievement* (*)(void*, int)>(vtable[0]);
    return get_achievement_by_index_fn(this, index);
  }

  int get_achievement_count()
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto get_achievement_count_fn = reinterpret_cast<int (*)(void*)>(vtable[2]);
    return get_achievement_count_fn(this);
  }

  void award_achievement(int achievement_id)
  {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto award_achievement_fn = reinterpret_cast<void (*)(void*, int)>(vtable[4]);
    award_achievement_fn(this, achievement_id);
  }
};

#endif
