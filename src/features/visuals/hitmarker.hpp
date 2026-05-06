/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/visuals/hitmarker.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef HITMARKER_HPP
#define HITMARKER_HPP

class Player;

namespace hitmarker
{

void on_player_hurt(Player* attacker, Player* victim, int damage, bool crit, bool headshot);
void draw_imgui();

} // namespace hitmarker

#endif
