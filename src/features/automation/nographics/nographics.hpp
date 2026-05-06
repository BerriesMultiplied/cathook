/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/automation/nographics/nographics.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef NOGRAPHICS_HPP
#define NOGRAPHICS_HPP

namespace nographics
{

void initialize();
void prepare_render_patches();
void update();
void shutdown();
bool should_skip_rendering_hooks();

} // namespace nographics

#endif
