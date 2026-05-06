/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/visuals/hitmarker.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/visuals/hitmarker.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "imgui/dearimgui.hpp"

#include "features/menu/config.hpp"
#include "features/visuals/overlay_projection.hpp"

#include "games/tf2/sdk/entities/player.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/global_vars.hpp"

namespace hitmarker
{

namespace
{

struct hit_entry
{
  float time = 0.0f;
  int damage = 0;
  bool crit = false;
  bool headshot = false;
  Vec3 world_position{};
};

std::vector<hit_entry>& hits()
{
  static std::vector<hit_entry> value{};
  return value;
}

ImU32 to_imgui_color(const RGBA_float& color, float alpha_scale = 1.0f)
{
  auto mutable_color = color;
  const auto rgba = mutable_color.to_RGBA();
  const int alpha = std::clamp(static_cast<int>(std::round(static_cast<float>(rgba.a) * alpha_scale)), 0, 255);
  return IM_COL32(rgba.r, rgba.g, rgba.b, alpha);
}

float hit_progress(const hit_entry& hit)
{
  if (global_vars == nullptr || config.visuals.hitmarker.duration <= 0.0f) {
    return 1.0f;
  }

  return (global_vars->curtime - hit.time) / config.visuals.hitmarker.duration;
}

bool hit_expired(const hit_entry& hit)
{
  return hit_progress(hit) >= 1.0f;
}

void prune_hits()
{
  auto& entries = hits();
  std::erase_if(entries, [](const hit_entry& hit) { return hit_expired(hit); });
}

} // namespace

void on_player_hurt(Player* attacker, Player* victim, int damage, bool crit, bool headshot)
{
  if (!config.visuals.hitmarker.enabled || attacker == nullptr || victim == nullptr || entity_list == nullptr || global_vars == nullptr) {
    return;
  }

  auto* localplayer = entity_list->get_localplayer();
  if (localplayer == nullptr || attacker != localplayer || victim == localplayer) {
    return;
  }

  auto& entries = hits();
  entries.push_back(hit_entry{
    .time = global_vars->curtime,
    .damage = std::max(0, damage),
    .crit = crit,
    .headshot = headshot,
    .world_position = victim->get_origin() + Vec3{0.0f, 0.0f, 72.0f},
  });
}

void draw_imgui()
{
  if (!config.visuals.hitmarker.enabled || global_vars == nullptr) {
    return;
  }

  prune_hits();
  auto& entries = hits();
  if (entries.empty()) {
    return;
  }

  auto* draw_list = ImGui::GetForegroundDrawList();
  const ImVec2 display_center(
    ImGui::GetIO().DisplaySize.x * 0.5f,
    ImGui::GetIO().DisplaySize.y * 0.5f);

  float strongest_alpha = 0.0f;
  for (const auto& hit : entries) {
    strongest_alpha = std::max(strongest_alpha, 1.0f - std::clamp(hit_progress(hit), 0.0f, 1.0f));
  }

  if (strongest_alpha > 0.0f) {
    const float size = config.visuals.hitmarker.size;
    const ImU32 line_color = to_imgui_color(config.visuals.hitmarker.color, strongest_alpha);
    const ImU32 outline_color = IM_COL32(0, 0, 0, std::clamp(static_cast<int>(std::round(190.0f * strongest_alpha)), 0, 255));

    draw_list->AddLine(ImVec2(display_center.x - size, display_center.y - size), ImVec2(display_center.x - 2.0f, display_center.y - 2.0f), outline_color, 3.0f);
    draw_list->AddLine(ImVec2(display_center.x + size, display_center.y - size), ImVec2(display_center.x + 2.0f, display_center.y - 2.0f), outline_color, 3.0f);
    draw_list->AddLine(ImVec2(display_center.x - size, display_center.y + size), ImVec2(display_center.x - 2.0f, display_center.y + 2.0f), outline_color, 3.0f);
    draw_list->AddLine(ImVec2(display_center.x + size, display_center.y + size), ImVec2(display_center.x + 2.0f, display_center.y + 2.0f), outline_color, 3.0f);

    draw_list->AddLine(ImVec2(display_center.x - size, display_center.y - size), ImVec2(display_center.x - 2.0f, display_center.y - 2.0f), line_color, 1.25f);
    draw_list->AddLine(ImVec2(display_center.x + size, display_center.y - size), ImVec2(display_center.x + 2.0f, display_center.y - 2.0f), line_color, 1.25f);
    draw_list->AddLine(ImVec2(display_center.x - size, display_center.y + size), ImVec2(display_center.x - 2.0f, display_center.y + 2.0f), line_color, 1.25f);
    draw_list->AddLine(ImVec2(display_center.x + size, display_center.y + size), ImVec2(display_center.x + 2.0f, display_center.y + 2.0f), line_color, 1.25f);
  }

  if (!config.visuals.hitmarker.damage_text || !overlay_projection::begin_frame()) {
    return;
  }

  for (const auto& hit : entries) {
    Vec3 screen{};
    Vec3 position = hit.world_position;
    const float progress = std::clamp(hit_progress(hit), 0.0f, 1.0f);
    position.z += progress * 18.0f;

    if (!overlay_projection::world_to_screen(position, &screen)) {
      continue;
    }

    const float alpha = 1.0f - progress;
    const RGBA_float& color =
      hit.headshot ? config.visuals.hitmarker.headshot_color :
      hit.crit ? config.visuals.hitmarker.crit_color :
      config.visuals.hitmarker.color;
    const std::string text = std::to_string(hit.damage);
    const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    const ImVec2 text_pos(screen.x - (text_size.x * 0.5f), screen.y - text_size.y);
    const ImU32 outline_color = IM_COL32(0, 0, 0, std::clamp(static_cast<int>(std::round(alpha * 190.0f)), 0, 255));

    draw_list->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), outline_color, text.c_str());
    draw_list->AddText(text_pos, to_imgui_color(color, alpha), text.c_str());
  }
}

} // namespace hitmarker
