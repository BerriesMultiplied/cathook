/*
/^-----^\   data: 2026-04-05
V  o o  V  file: src/features/automation/navbot/navbot_debug.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef NAVBOT_DEBUG_HPP
#define NAVBOT_DEBUG_HPP

#include "features/automation/navbot/navbot_types.hpp"

struct ImDrawList;

namespace navbot
{

void draw_path_exact_imgui(ImDrawList* draw_list, const path_result& path, size_t current_crumb_index, float current_time, const std::vector<float>& reached_crumb_times);
void draw_debug_overlay_imgui(ImDrawList* draw_list, const navbot_debug_state& debug_state);

} // namespace navbot

#endif
