/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/class_menu_show_panel.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include <unistd.h>

#include "games/tf2/sdk/interfaces/entity_list.hpp"

#include "features/menu/config.hpp"

void (*class_menu_show_panel_original)(void*, bool) = NULL;

void class_menu_show_panel_hook(void* me, bool show) {
  Player* localplayer = entity_list->get_localplayer();
  if (localplayer == nullptr) {
    class_menu_show_panel_original(me, show);
    return;
  }
  
  if (config.misc.automation.auto_class_select == true  && localplayer->get_tf_class() == tf_class::UNDEFINED) {
    class_menu_show_panel_original(me, false);
  } else {
    class_menu_show_panel_original(me, show);
  }
}
