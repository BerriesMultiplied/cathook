/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/team_menu_show_panel.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include <unistd.h>

#include "features/menu/config.hpp"
#include "games/tf2/sdk/interfaces/engine.hpp"

#include "core/print.hpp"

void (*team_menu_show_panel_original)(void*, bool) = NULL;

void team_menu_show_panel_hook(void* me, bool show) {
  Player* localplayer = entity_list->get_localplayer();
  if (localplayer == nullptr) {
    team_menu_show_panel_original(me, show);
    return;
  }

  if (config.misc.automation.auto_class_select == true && localplayer->get_team() == tf_team::UNKNOWN) {
    team_menu_show_panel_original(me, false);    
  } else {
    team_menu_show_panel_original(me, show);
  }  
}
