/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/should_draw_local_player.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "games/tf2/sdk/interfaces/entity_list.hpp"

#include "games/tf2/sdk/interfaces/input.hpp"

#include "games/tf2/sdk/entities/player.hpp"
#include "features/visuals/thirdperson.hpp"

bool (*should_draw_local_player_original)(void*);

bool should_draw_local_player_hook(void* me) {
  Player* localplayer = entity_list->get_localplayer();
  if (localplayer != nullptr && me == localplayer && thirdperson::should_draw_local_player()) {
    return true;
  }

  return should_draw_local_player_original(me);
}
