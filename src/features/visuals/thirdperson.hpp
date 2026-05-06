/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/visuals/thirdperson.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef THIRDPERSON_HPP
#define THIRDPERSON_HPP

struct Player;
struct view_setup;

namespace thirdperson
{

[[nodiscard]] bool should_draw_local_player();
void update_taunt_camera();
void update_camera(view_setup* setup);
void begin_render_angles();
void end_render_angles();
void draw_crosshair_imgui();

} // namespace thirdperson

#endif
