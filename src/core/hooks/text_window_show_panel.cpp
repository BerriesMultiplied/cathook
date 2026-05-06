/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/text_window_show_panel.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

//55 48 8D 15 ? ? ? ? 48 89 E5 41 54 41 89 F4 53 48 8B 07

#include <unistd.h>

#include "features/menu/config.hpp"

void (*text_window_show_panel_original)(void*, bool) = NULL;

void text_window_show_panel_hook(void* me, bool show) {
  if (config.misc.automation.anti_motd == true) {
    text_window_show_panel_original(me, false);
  } else {
    text_window_show_panel_original(me, show);
  }    
}
