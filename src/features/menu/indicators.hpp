/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/menu/indicators.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef INDICATORS_HPP
#define INDICATORS_HPP

#include "config.hpp"
#include "binds.hpp"
#include "menu.hpp"
#include "features/combat/random_crits/crit_hack.hpp"

#include "features/combat/aimbot/aimbot_debug.hpp"
#include "features/combat/tickbase/tickbase.hpp"
#include "features/visuals/spectator_list.hpp"

#include "games/tf2/sdk/entities/player.hpp"
#include "games/tf2/sdk/interfaces/engine.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"

#include "imgui/dearimgui.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace cat_indicator
{

enum class section_kind
{
  tickbase,
  keybinds,
  spectators,
  aimbot_debug,
  crit_hack
};

struct section_spec
{
  section_kind kind = section_kind::keybinds;
  float width = 0.0f;
  float height = 0.0f;
  ImVec2 position{};
};

inline bool should_draw_overlay()
{
  if (menu_focused) {
    return true;
  }

  if (engine != nullptr && !engine->is_in_game()) {
    return false;
  }

  if (entity_list == nullptr) {
    return true;
  }

  Player* localplayer = entity_list->get_localplayer();
  return localplayer == nullptr || localplayer->is_alive();
}

inline bool has_indicator(const uint32_t flag)
{
  return (config.visuals.indicators.enabled_mask & flag) != 0;
}

inline ImU32 spectator_name_color(const spectator_list::spectator_entry& spectator)
{
  if (config.visuals.spectator_list.highlight_firstperson && spectator.firstperson) {
    auto firstperson_color = config.visuals.spectator_list.firstperson_color.to_RGBA();
    return IM_COL32(firstperson_color.r, firstperson_color.g, firstperson_color.b, firstperson_color.a);
  }

  return ImGui::GetColorU32(cat_menu::k_text);
}

inline void draw_compact_meter(
  ImDrawList* draw_list,
  const ImVec2 position,
  const float container_width,
  const std::string& left_text,
  const std::string& right_text,
  const float progress,
  const ImU32 bar_color)
{
  constexpr ImVec2 size(180.0f, 29.0f);
  constexpr float bar_height = 3.0f;
  const float text_height = size.y - bar_height;
  const float box_x = position.x + ((container_width - size.x) * 0.5f);
  const ImVec2 box_position(box_x, position.y);
  const ImU32 background_color = IM_COL32(0, 0, 0, 180);
  const ImU32 text_color = IM_COL32(255, 255, 255, 255);

  draw_list->AddRectFilled(box_position, ImVec2(box_position.x + size.x, box_position.y + text_height), background_color, 0.0f);
  draw_list->AddRectFilled(ImVec2(box_position.x, box_position.y + text_height), ImVec2(box_position.x + size.x, box_position.y + size.y), background_color, 0.0f);

  const float clamped_progress = std::clamp(progress, 0.0f, 1.0f);
  const float fill_width = size.x * clamped_progress;
  if (fill_width > 0.0f) {
    draw_list->AddRectFilled(
      ImVec2(box_position.x, box_position.y + text_height),
      ImVec2(box_position.x + fill_width, box_position.y + size.y),
      bar_color,
      0.0f);
  }

  draw_list->AddText(
    ImVec2(box_position.x + 5.0f, box_position.y + ((text_height - ImGui::GetTextLineHeight()) * 0.5f)),
    text_color,
    left_text.c_str());

  if (!right_text.empty()) {
    const ImVec2 right_size = ImGui::CalcTextSize(right_text.c_str());
    draw_list->AddText(
      ImVec2(box_position.x + size.x - right_size.x - 5.0f, box_position.y + ((text_height - ImGui::GetTextLineHeight()) * 0.5f)),
      bar_color,
      right_text.c_str());
  }
}

inline void draw_panel_box(ImDrawList* draw_list, const ImVec2 position, const ImVec2 size)
{
  draw_list->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), ImGui::GetColorU32(cat_menu::with_alpha(cat_menu::k_bg_panel, 0.96f)), 0.0f);
  draw_list->AddRect(position, ImVec2(position.x + size.x, position.y + size.y), ImGui::GetColorU32(cat_menu::k_line), 0.0f, 0, 1.0f);
}

inline auto collect_keybind_rows() -> std::vector<cat_bind::indicator_row>
{
  std::vector<cat_bind::indicator_row> rows = cat_bind::collect_indicator_rows();
  if (rows.empty() && menu_focused) {
    rows.push_back({
      .label = "No binds",
      .key = "",
      .state = "",
      .target_key = "",
      .popup_type = cat_bind::popup_target_type::value_bind,
      .active = false
    });
  }

  return rows;
}

inline auto collect_spectator_rows(Player** target_player_out) -> std::vector<spectator_list::spectator_entry>
{
  std::vector<spectator_list::spectator_entry> spectators = spectator_list::collect_spectators(target_player_out);
  if (spectators.empty() && menu_focused) {
    spectators.push_back({
      .name = "No spectators",
      .firstperson = false
    });
  }

  return spectators;
}

inline auto build_sections() -> std::vector<section_spec>
{
  std::vector<section_spec> sections{};
  sections.reserve(4);

  if (has_indicator(Visuals::Indicators::crit_hack) && (menu_focused || config.crithack.enabled)) {
    sections.push_back({
      .kind = section_kind::crit_hack,
      .width = 180.0f,
      .height = 29.0f,
      .position = ImVec2(config.visuals.indicators.crit_hack_x, config.visuals.indicators.crit_hack_y)
    });
  }

  if (has_indicator(Visuals::Indicators::tickbase) && (menu_focused || config.misc.exploits.tickbase)) {
    sections.push_back({
      .kind = section_kind::tickbase,
      .width = 180.0f,
      .height = 29.0f,
      .position = ImVec2(config.visuals.indicators.legacy_ticks_x, config.visuals.indicators.legacy_ticks_y)
    });
  }

  if (has_indicator(Visuals::Indicators::keybinds)) {
    const std::vector<cat_bind::indicator_row> rows = collect_keybind_rows();
    if (!rows.empty()) {
      sections.push_back({
        .kind = section_kind::keybinds,
        .width = 240.0f,
        .height = 28.0f + (18.0f * static_cast<float>(rows.size())) + 8.0f,
        .position = ImVec2(config.visuals.indicators.keybinds_x, config.visuals.indicators.keybinds_y)
      });
    }
  }

  if (has_indicator(Visuals::Indicators::spectators)) {
    Player* target_player = nullptr;
    const std::vector<spectator_list::spectator_entry> spectators = collect_spectator_rows(&target_player);
    if (!spectators.empty()) {
      sections.push_back({
        .kind = section_kind::spectators,
        .width = 250.0f,
        .height = 28.0f + (18.0f * static_cast<float>(spectators.size())) + 8.0f,
        .position = ImVec2(config.visuals.spectator_list.x, config.visuals.spectator_list.y)
      });
    }
  }

  if (config.aimbot.debug_overlay) {
    const aimbot_debug_state& state = aimbot_debug_get_state();
    if (menu_focused || state.active) {
      sections.push_back({
        .kind = section_kind::aimbot_debug,
        .width = 360.0f,
        .height = 310.0f,
        .position = ImVec2(config.aimbot.debug_overlay_x, config.aimbot.debug_overlay_y)
      });
    }
  }

  return sections;
}

struct section_position_refs
{
  float* x = nullptr;
  float* y = nullptr;
};

inline auto section_drag_position(section_kind kind) -> section_position_refs
{
  switch (kind) {
  case section_kind::crit_hack:
    return { .x = &config.visuals.indicators.crit_hack_x, .y = &config.visuals.indicators.crit_hack_y };
  case section_kind::tickbase:
    return { .x = &config.visuals.indicators.legacy_ticks_x, .y = &config.visuals.indicators.legacy_ticks_y };
  case section_kind::keybinds:
    return { .x = &config.visuals.indicators.keybinds_x, .y = &config.visuals.indicators.keybinds_y };
  case section_kind::spectators:
    return { .x = &config.visuals.spectator_list.x, .y = &config.visuals.spectator_list.y };
  case section_kind::aimbot_debug:
    return { .x = &config.aimbot.debug_overlay_x, .y = &config.aimbot.debug_overlay_y };
  }

  return {};
}

inline void handle_drag_window(const section_spec& section)
{
  if (!menu_focused) {
    return;
  }

  const section_position_refs position = section_drag_position(section.kind);
  if (position.x == nullptr || position.y == nullptr) {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(*position.x, *position.y), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(section.width, section.height), ImGuiCond_Always);
  constexpr ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoNav;

  const std::string window_name = "indicator_drag_" + std::to_string(static_cast<int>(section.kind));
  if (!ImGui::Begin(window_name.c_str(), nullptr, flags)) {
    ImGui::End();
    return;
  }

  const float drag_height = section.kind == section_kind::keybinds || section.kind == section_kind::spectators ? 28.0f : section.height;
  ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
  ImGui::InvisibleButton("##indicator_drag", ImVec2(section.width, drag_height));
  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    const ImVec2 delta = ImGui::GetIO().MouseDelta;
    *position.x += delta.x;
    *position.y += delta.y;
  }

  if (section.kind == section_kind::keybinds) {
    const std::vector<cat_bind::indicator_row> rows = collect_keybind_rows();
    constexpr float row_height = 18.0f;
    constexpr float header_height = 28.0f;
    float row_y = header_height;
    for (std::size_t index = 0; index < rows.size(); ++index) {
      const auto& row = rows[index];
      if (row.target_key.empty()) {
        row_y += row_height;
        continue;
      }

      ImGui::SetCursorPos(ImVec2(0.0f, row_y));
      ImGui::PushID(static_cast<int>(index));
      ImGui::InvisibleButton("##keybind_indicator_row", ImVec2(section.width, row_height), ImGuiButtonFlags_MouseButtonRight);
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        cat_bind::request_popup(row.target_key, row.popup_type);
      }
      ImGui::PopID();
      row_y += row_height;
    }
  }

  ImGui::End();
}

inline auto format_float(const float value, const char* format) -> std::string
{
  char buffer[64]{};
  std::snprintf(buffer, sizeof(buffer), format, value);
  return buffer;
}

inline void draw_info_row(ImDrawList* draw_list, const ImVec2 position, const float width, const char* label, const std::string& value, const ImU32 value_color)
{
  const ImU32 label_color = ImGui::GetColorU32(cat_menu::k_text_muted);
  draw_list->AddText(ImVec2(position.x + 12.0f, position.y), label_color, label);

  std::string clipped_value = value;
  const float max_width = width - 96.0f;
  while (!clipped_value.empty() && ImGui::CalcTextSize(clipped_value.c_str()).x > max_width) {
    clipped_value.pop_back();
  }
  if (clipped_value.size() < value.size() && clipped_value.size() > 3) {
    clipped_value.resize(clipped_value.size() - 3);
    clipped_value += "...";
  }

  const ImVec2 value_size = ImGui::CalcTextSize(clipped_value.c_str());
  draw_list->AddText(ImVec2(position.x + width - value_size.x - 12.0f, position.y), value_color, clipped_value.c_str());
}

inline auto bool_text(const bool value) -> const char*
{
  return value ? "yes" : "no";
}

inline auto format_optional_float(const float value, const char* format) -> std::string
{
  if (!std::isfinite(value) || value >= FLT_MAX * 0.5f) {
    return "n/a";
  }

  return format_float(value, format);
}

inline auto format_reject_summary(const aimbot_reject_debug& reject) -> std::string
{
  if (reject.reason == aimbot_reject_reason::none) {
    return "none";
  }

  std::string text = "#" + std::to_string(reject.entity_index) + " ";
  text += aimbot_debug_reject_reason_name(reject.reason);
  if (reject.hitbox >= 0) {
    text += " hb " + std::to_string(reject.hitbox);
  }
  if (reject.team >= 0) {
    text += " t" + std::to_string(reject.team);
  }
  if (reject.health > 0) {
    text += " hp " + std::to_string(reject.health);
  }
  return text;
}

inline auto format_reject_detail(const aimbot_reject_debug& reject) -> std::string
{
  if (reject.reason == aimbot_reject_reason::none) {
    return "none";
  }

  std::string text = "fov " + format_optional_float(reject.fov, "%.2f") + "/" + format_optional_float(reject.fov_limit, "%.2f");
  text += " d " + format_optional_float(reject.distance, "%.0f");
  text += " vis " + std::string(bool_text(reject.visible));
  if (reject.trace_entity_index >= 0 || reject.trace_hitbox >= 0) {
    text += " tr " + std::to_string(reject.trace_entity_index) + " hb " + std::to_string(reject.trace_hitbox);
  }
  if (reject.preferred) {
    text += " pref";
  }
  if (reject.current) {
    text += " cur";
  }
  if (reject.backtrack) {
    text += " bt";
  }
  return text;
}

inline void draw_aimbot_debug_section(ImDrawList* draw_list, const ImVec2 position)
{
  const aimbot_debug_state& state = aimbot_debug_get_state();
  constexpr float width = 360.0f;
  constexpr float height = 310.0f;
  constexpr float row_height = 17.0f;
  draw_panel_box(draw_list, position, ImVec2(width, height));

  const ImVec2 title_size = ImGui::CalcTextSize("Aimbot");
  draw_list->AddText(ImVec2(position.x + 8.0f, position.y + 7.0f), ImGui::GetColorU32(cat_menu::k_text), "Aimbot");
  draw_list->AddText(
    ImVec2(position.x + 8.0f + title_size.x, position.y + 7.0f),
    state.attack_ready ? ImGui::GetColorU32(cat_menu::k_accent) : IM_COL32(255, 150, 0, 255),
    " debug");

  const ImU32 value_color = state.attack_ready
    ? ImGui::GetColorU32(cat_menu::k_accent)
    : IM_COL32(255, 150, 0, 255);
  float row_y = position.y + 31.0f;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "reason", aimbot_debug_reason_name(state.reason), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "target", std::to_string(state.selected_entity_index) + " hb " + std::to_string(state.selected_hitbox), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "trace", std::to_string(state.trace_entity_index) + " hb " + std::to_string(state.trace_hitbox), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "candidates", std::to_string(state.candidates_visible) + "/" + std::to_string(state.candidates_total) + " reject " + std::to_string(state.candidates_rejected), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "last reject", format_reject_summary(state.last_reject), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "last detail", format_reject_detail(state.last_reject), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "best reject", format_reject_summary(state.best_reject), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "best detail", format_reject_detail(state.best_reject), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "skip ig/fr/ip/cl/tm/in/de/type", std::to_string(state.skipped_ignored) + "/" + std::to_string(state.skipped_friends) + "/" + std::to_string(state.skipped_ipc) + "/" + std::to_string(state.skipped_cloaked) + "/" + std::to_string(state.skipped_team) + "/" + std::to_string(state.skipped_invulnerable) + "/" + std::to_string(state.skipped_dead) + "/" + std::to_string(state.skipped_type), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "resolver", std::string(state.resolver_active ? "on " : "off ") + aimbot_debug_resolver_mode_name(state.resolver_mode) + " y" + std::to_string(state.resolver_candidates), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "res angle", format_float(state.resolver_yaw, "%.1f") + " / " + format_float(state.resolver_pitch, "%.1f"), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "res hit/miss", std::to_string(state.resolver_hits) + " / " + std::to_string(state.resolver_misses), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "scope/head", std::string(bool_text(state.scoped_ready)) + " / " + bool_text(state.headshot_ready), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "spread", format_float(state.spread, "%.4f") + (state.spread_compensated ? " comp" : ""), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "pellet", std::to_string(state.pellet_index) + " / " + std::to_string(state.pellet_count), value_color);
  row_y += row_height;
  draw_info_row(draw_list, ImVec2(position.x, row_y), width, "tick/fov", std::to_string(state.tick_count) + " / " + format_float(state.fov, "%.2f"), value_color);
}

inline auto tickbase_indicator_color(const tickbase::indicator_state& state) -> ImU32
{
  if (!config.misc.exploits.tickbase) {
    return ImGui::GetColorU32(cat_menu::k_text_muted);
  }

  if (state.shifting || state.doubletap || state.warp) {
    return ImGui::GetColorU32(cat_menu::k_accent);
  }

  if (state.recharging) {
    return IM_COL32(255, 150, 0, 255);
  }

  if (state.available_shift_ticks > 0) {
    return IM_COL32(100, 220, 130, 255);
  }

  return ImGui::GetColorU32(cat_menu::k_text_muted);
}

inline auto build_tickbase_status_text(const tickbase::indicator_state& state) -> std::string
{
  if (!config.misc.exploits.tickbase) {
    return "off";
  }

  if (state.doubletap) {
    return "doubletap";
  }

  if (state.warp) {
    return "warp";
  }

  if (state.recharging) {
    return "recharge";
  }

  if (state.fakelag && state.choked_commands > 0) {
    return "fakelag";
  }

  return state.available_shift_ticks > 0 ? "ready" : "building";
}

inline void draw_tickbase_section(ImDrawList* draw_list, const ImVec2 position)
{
  const auto state = tickbase::get_indicator_state();
  const auto bar_color = tickbase_indicator_color(state);
  const auto status = build_tickbase_status_text(state);
  const int max_ticks = std::max(1, state.max_processing_ticks);
  const auto tick_text = std::to_string(std::clamp(state.processing_ticks, 0, max_ticks)) + "/" + std::to_string(max_ticks);
  const float progress = static_cast<float>(std::clamp(state.processing_ticks, 0, max_ticks)) / static_cast<float>(max_ticks);

  draw_compact_meter(draw_list, position, 180.0f, status, tick_text, progress, bar_color);
}

inline void draw_crit_hack_section(ImDrawList* draw_list, const ImVec2 position)
{
  auto* local = entity_list->get_localplayer();
  if (local == nullptr || !local->is_alive() || local->is_dormant()) {
    return;
  }

  auto* weapon = local->get_weapon();
  if (weapon == nullptr || !crit_hack::weapon_can_crit(weapon, true)) {
    return;
  }

  const auto stats = crit_hack::get_stats();
  std::string left_text = "Calculating";
  std::string right_text = "";
  float progress = 0.0f;
  ImU32 bar_color = IM_COL32(100, 220, 130, 255); // default ready green

  if (!crit_hack::weapon_can_crit(weapon)) {
    left_text = "Cannot crit";
    right_text = "DISABLED";
    bar_color = IM_COL32(200, 40, 40, 255); // dark red
    progress = 1.0f;
  } else {
    if (stats.damage > 0) {
      if (local->is_crit_boosted()) {
        left_text = "Crit Boosted";
        right_text = "READY";
        bar_color = IM_COL32(100, 255, 255, 255); // cyan
        progress = 1.0f;
      } else if (weapon->crit_time() > global_vars->curtime) {
        const float flTime = weapon->crit_time() - global_vars->curtime;
        left_text = "Crits: " + std::to_string(std::max(0, stats.available)) + " / " + std::to_string(stats.potential);
        right_text = "STREAMING";
        bar_color = IM_COL32(100, 255, 255, 255);
        progress = flTime / 2.0f;
      } else if (!stats.banned || weapon->is_melee()) {
        left_text = "Crits: " + std::to_string(std::max(0, stats.available)) + " / " + std::to_string(stats.potential);
        
        if (weapon->is_rapid_fire() && (local->get_tickbase() * 0.015f) < weapon->last_rapid_fire_crit_check_time() + 1.0f) {
          const float flTime = weapon->last_rapid_fire_crit_check_time() + 1.0f - (local->get_tickbase() * 0.015f);
          if (flTime > 0.0001f) {
            char wait_buf[32];
            std::snprintf(wait_buf, sizeof(wait_buf), "WAIT %.2fs", flTime);
            right_text = wait_buf;
            bar_color = IM_COL32(255, 150, 0, 255); // orange
          } else {
            right_text = "STREAMING";
            bar_color = IM_COL32(100, 255, 255, 255);
          }
          progress = flTime;
        } else if (stats.available >= stats.potential) {
          right_text = "READY";
          bar_color = IM_COL32(100, 220, 130, 255);
          progress = 1.0f;
        } else {
          float currentBucket = weapon->crit_token_bucket();
          int damageNeeded = static_cast<int>(std::ceil(stats.cost - currentBucket));
          right_text = "DMG: " + std::to_string(std::max(0, damageNeeded));
          bar_color = stats.available > 0 ? IM_COL32(100, 220, 130, 255) : IM_COL32(200, 40, 40, 255);
          
          static Convar* cap_cvar = nullptr;
          if (cap_cvar == nullptr && convar_system != nullptr) {
            cap_cvar = convar_system->find_var("tf_weapon_criticals_bucket_cap");
          }
          float cap = cap_cvar ? cap_cvar->get_float() : 1000.0f;
          progress = currentBucket / cap;
        }
      } else {
        left_text = "DMG: " + std::to_string(static_cast<int>(std::ceil(stats.damage_till_flip)));
        right_text = "BANNED";
        bar_color = IM_COL32(200, 40, 40, 255);
        progress = 0.2f;
      }
    }
  }

  draw_compact_meter(draw_list, position, 180.0f, left_text, right_text, progress, bar_color);
}

inline void draw_keybind_section(ImDrawList* draw_list, const ImVec2 position)
{
  const std::vector<cat_bind::indicator_row> rows = collect_keybind_rows();
  if (rows.empty()) {
    return;
  }

  constexpr float width = 240.0f;
  constexpr float row_height = 18.0f;
  constexpr float header_height = 28.0f;
  const float height = header_height + (row_height * static_cast<float>(rows.size())) + 8.0f;
  draw_panel_box(draw_list, position, ImVec2(width, height));

  const ImVec2 key_size = ImGui::CalcTextSize("Key");
  draw_list->AddText(ImVec2(position.x + 8.0f, position.y + 7.0f), ImGui::GetColorU32(cat_menu::k_text), "Key");
  draw_list->AddText(ImVec2(position.x + 8.0f + key_size.x, position.y + 7.0f), ImGui::GetColorU32(cat_menu::k_accent), "binds");

  float row_y = position.y + header_height;
  for (const cat_bind::indicator_row& row : rows) {
    const ImU32 row_name_color = row.active ? ImGui::GetColorU32(cat_menu::k_accent) : ImGui::GetColorU32(cat_menu::k_text);
    const ImU32 row_text_color = row.active ? ImGui::GetColorU32(cat_menu::k_text) : ImGui::GetColorU32(cat_menu::k_text_muted);
    const ImVec2 state_size = ImGui::CalcTextSize(row.state.c_str());
    const float key_x = position.x + 104.0f;
    const float state_x = position.x + width - state_size.x - 12.0f;
    const float max_key_width = std::max(0.0f, state_x - key_x - 8.0f);
    std::string key_text = row.key;

    while (!key_text.empty() && ImGui::CalcTextSize(key_text.c_str()).x > max_key_width) {
      key_text.pop_back();
    }
    if (key_text.size() < row.key.size() && key_text.size() > 3) {
      key_text.resize(key_text.size() - 3);
      key_text += "...";
    }

    draw_list->AddText(ImVec2(position.x + 12.0f, row_y), row_name_color, row.label.c_str());
    draw_list->AddText(ImVec2(key_x, row_y), row_text_color, key_text.c_str());
    draw_list->AddText(ImVec2(state_x, row_y), row.active ? ImGui::GetColorU32(cat_menu::k_accent) : ImGui::GetColorU32(cat_menu::k_text_muted), row.state.c_str());
    row_y += row_height;
  }
}

inline auto build_spectator_title(Player* target_player) -> std::string
{
  std::string title = "Spectators";
  if (!config.visuals.spectator_list.show_target || target_player == nullptr || engine == nullptr || entity_list == nullptr) {
    return title;
  }

  player_info info{};
  if (!engine->get_player_info(target_player->get_index(), &info)) {
    return title;
  }

  title += ": ";
  title += target_player == entity_list->get_localplayer() ? "you" : info.name;
  return title;
}

inline void draw_spectator_section(ImDrawList* draw_list, const ImVec2 position)
{
  Player* target_player = nullptr;
  const std::vector<spectator_list::spectator_entry> spectators = collect_spectator_rows(&target_player);
  if (spectators.empty()) {
    return;
  }

  constexpr float width = 250.0f;
  constexpr float row_height = 18.0f;
  constexpr float header_height = 28.0f;
  const float height = header_height + (row_height * static_cast<float>(spectators.size())) + 8.0f;
  draw_panel_box(draw_list, position, ImVec2(width, height));

  const std::string title = build_spectator_title(target_player);
  draw_list->AddText(ImVec2(position.x + 10.0f, position.y + 7.0f), ImGui::GetColorU32(cat_menu::k_text), title.c_str());

  float row_y = position.y + header_height;
  for (const spectator_list::spectator_entry& spectator : spectators) {
    const ImU32 name_color = spectator_name_color(spectator);
    draw_list->AddText(ImVec2(position.x + 12.0f, row_y), name_color, spectator.name.c_str());

    if (config.visuals.spectator_list.show_modes) {
      const char* mode_text = spectator.firstperson ? "1st" : "3rd";
      const ImVec2 mode_size = ImGui::CalcTextSize(mode_text);
      draw_list->AddText(
        ImVec2(position.x + width - mode_size.x - 12.0f, row_y),
        spectator.firstperson ? name_color : ImGui::GetColorU32(cat_menu::k_text_muted),
        mode_text);
    }

    row_y += row_height;
  }
}

} // namespace cat_indicator

static void draw_game_indicators()
{
  using namespace cat_indicator;

  if (!should_draw_overlay()) {
    return;
  }

  const std::vector<section_spec> sections = build_sections();
  if (sections.empty()) {
    return;
  }

  for (const section_spec& section : sections) {
    handle_drag_window(section);
  }

  ImDrawList* draw_list = ImGui::GetForegroundDrawList();
  for (const section_spec& section : sections) {
    switch (section.kind) {
    case section_kind::crit_hack:
      draw_crit_hack_section(draw_list, section.position);
      break;
    case section_kind::tickbase:
      draw_tickbase_section(draw_list, section.position);
      break;
    case section_kind::keybinds:
      draw_keybind_section(draw_list, section.position);
      break;
    case section_kind::spectators:
      draw_spectator_section(draw_list, section.position);
      break;
    case section_kind::aimbot_debug:
      draw_aimbot_debug_section(draw_list, section.position);
      break;
    }
  }
}

#endif
