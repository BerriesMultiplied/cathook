/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/menu/binds.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef BINDS_HPP
#define BINDS_HPP

#include "config.hpp"
#include "core/config/config_store.hpp"
#include "imgui/dearimgui.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace cat_bind
{

enum class value_type
{
  boolean,
  integer,
  floating
};

enum class widget_type
{
  checkbox,
  combo_int,
  slider_int,
  slider_float
};

enum class indicator_kind
{
  value,
  aimbot
};

using bind_value = std::variant<bool, int, float>;

struct bind_entry
{
  std::string target_key{};
  std::string label{};
  std::string default_label{};
  void* target = nullptr;
  value_type type = value_type::boolean;
  widget_type widget = widget_type::checkbox;
  button trigger{};
  bool enabled = true;
  bool show_in_indicator = true;
  bool active = false;
  bool has_default = false;
  bool has_override = false;
  bind_value default_value = false;
  bind_value override_value = false;
  int int_min = 0;
  int int_max = 0;
  float float_min = 0.0f;
  float float_max = 0.0f;
  std::string format{};
  std::vector<std::pair<std::string, int>> options{};
};

struct button_entry
{
  std::string target_key{};
  std::string label{};
  std::string default_label{};
  button* target = nullptr;
  indicator_kind kind = indicator_kind::value;
  bool show_in_indicator = true;
};

enum class popup_target_type
{
  value_bind,
  button_bind
};

struct indicator_row
{
  std::string label{};
  std::string key{};
  std::string state{};
  std::string target_key{};
  popup_target_type popup_type = popup_target_type::value_bind;
  bool active = false;
};

inline std::vector<bind_entry>& entries() {
  static std::vector<bind_entry> value{};
  return value;
}

inline std::unordered_map<void*, std::string>& pointer_to_key() {
  static std::unordered_map<void*, std::string> value{};
  return value;
}

inline std::unordered_map<std::string, void*>& key_to_pointer() {
  static std::unordered_map<std::string, void*> value{};
  return value;
}

inline std::unordered_map<std::string, value_type>& key_to_type() {
  static std::unordered_map<std::string, value_type> value{};
  return value;
}

inline std::vector<button_entry>& button_entries() {
  static std::vector<button_entry> value{};
  return value;
}

inline bool& targets_registered() {
  static bool value = false;
  return value;
}

inline bool& button_targets_registered() {
  static bool value = false;
  return value;
}

inline std::recursive_mutex& bind_mutex() {
  static std::recursive_mutex value{};
  return value;
}

inline std::vector<std::string>& panel_label_stack() {
  static std::vector<std::string> value{};
  return value;
}

inline void push_panel_label(const std::string& name) {
  panel_label_stack().push_back(name);
}

inline void pop_panel_label() {
  if (!panel_label_stack().empty()) {
    panel_label_stack().pop_back();
  }
}

inline std::string current_panel_path() {
  std::string out{};
  for (const std::string& name : panel_label_stack()) {
    if (!out.empty()) out += '/';
    out += name;
  }
  return out;
}

inline std::string make_target_key_from_label(const char* label) {
  std::string base = current_panel_path();
  if (!base.empty()) {
    base += '/';
  }
  base += (label != nullptr ? label : "");
  for (char& c : base) {
    if (c == ' ') c = '_';
    else if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return base;
}

inline std::string& popup_target_key() {
  static std::string value{};
  return value;
}

inline bool& popup_open_requested() {
  static bool value = false;
  return value;
}

inline popup_target_type& popup_target_type_value() {
  static popup_target_type value = popup_target_type::value_bind;
  return value;
}

inline std::string& popup_label_target_key() {
  static std::string value{};
  return value;
}

inline std::array<char, 128>& popup_label_buffer() {
  static std::array<char, 128> value{};
  return value;
}

inline bind_entry* find_entry(const std::string& target_key) {
  for (bind_entry& entry : entries()) {
    if (entry.target_key == target_key) {
      return &entry;
    }
  }

  return nullptr;
}

inline button_entry* find_button_entry(const std::string& target_key) {
  for (button_entry& entry : button_entries()) {
    if (entry.target_key == target_key) {
      return &entry;
    }
  }

  return nullptr;
}

inline value_type get_value_type(bool*) { return value_type::boolean; }
inline value_type get_value_type(int*) { return value_type::integer; }
inline value_type get_value_type(float*) { return value_type::floating; }

inline bind_value read_value(void* target, value_type type) {
  switch (type) {
  case value_type::boolean:
    return *static_cast<bool*>(target);
  case value_type::integer:
    return *static_cast<int*>(target);
  case value_type::floating:
    return *static_cast<float*>(target);
  }

  return false;
}

inline void write_value(void* target, value_type type, const bind_value& value) {
  switch (type) {
  case value_type::boolean:
    *static_cast<bool*>(target) = std::get<bool>(value);
    break;
  case value_type::integer:
    *static_cast<int*>(target) = std::get<int>(value);
    break;
  case value_type::floating:
    *static_cast<float*>(target) = std::get<float>(value);
    break;
  }
}

inline const char* mode_label(const button::mode_type mode) {
  switch (mode) {
  case button::mode_type::HOLD:
    return "hold";
  case button::mode_type::TOGGLE:
    return "toggle";
  case button::mode_type::DOUBLE_CLICK:
    return "double";
  }

  return "hold";
}

template <typename value_t>
inline void register_target(const char* target_key, const char* label, value_t* target) {
  const value_type target_type = get_value_type(target);
  pointer_to_key()[target] = target_key;
  key_to_pointer()[target_key] = target;
  key_to_type()[target_key] = target_type;

  if (bind_entry* entry = find_entry(target_key)) {
    entry->target = target;
    if (entry->label.empty() || entry->label == entry->default_label) {
      entry->label = label;
    }
    entry->default_label = label;
    entry->type = target_type;
  }
}

inline void register_button_target(
  const char* target_key,
  const char* label,
  button* target,
  const indicator_kind kind,
  const bool show_in_indicator = true) {
  if (button_entry* entry = find_button_entry(target_key)) {
    entry->target = target;
    if (entry->label.empty() || entry->label == entry->default_label) {
      entry->label = label;
    }
    entry->default_label = label;
    entry->kind = kind;
    entry->show_in_indicator = show_in_indicator;
    return;
  }

  button_entries().push_back({
    .target_key = target_key,
    .label = label,
    .default_label = label,
    .target = target,
    .kind = kind,
    .show_in_indicator = show_in_indicator
  });
}

inline void register_builtin_targets() {
  std::lock_guard lock{bind_mutex()};

  if (targets_registered()) {
    return;
  }

  targets_registered() = true;
}

inline void register_builtin_button_targets() {
  std::lock_guard lock{bind_mutex()};

  if (button_targets_registered()) {
    return;
  }

  button_targets_registered() = true;

  register_button_target("aimbot.key", "Aimbot", &config.aimbot.key, indicator_kind::aimbot, true);
  register_button_target("misc.exploits.doubletap_key", "Doubletap", &config.misc.exploits.doubletap_key, indicator_kind::value, true);
  register_button_target("misc.exploits.warp_key", "Warp", &config.misc.exploits.warp_key, indicator_kind::value, true);
}

inline bind_entry& create_entry(const std::string& target_key, const char* label, void* target, value_type type) {
  bind_entry entry{};
  entry.target_key = target_key;
  entry.label = label;
  entry.default_label = label;
  entry.target = target;
  entry.type = type;
  entry.default_value = read_value(target, type);
  entry.override_value = entry.default_value;
  entry.has_default = true;
  entry.has_override = true;
  entries().push_back(entry);
  return entries().back();
}

template <typename value_t>
inline bind_entry* ensure_entry(value_t* target, const char* label) {
  register_builtin_targets();
  register_builtin_button_targets();

  if (target == nullptr) {
    return nullptr;
  }

  auto it = pointer_to_key().find(target);
  if (it == pointer_to_key().end()) {
    const std::string generated = make_target_key_from_label(label);
    register_target(generated.c_str(), label, target);
    it = pointer_to_key().find(target);
    if (it == pointer_to_key().end()) {
      return nullptr;
    }
  }

  bind_entry* entry = find_entry(it->second);
  if (entry == nullptr) {
    entry = &create_entry(it->second, label, target, get_value_type(target));
  }

  entry->target = target;
  entry->label = label;
  entry->type = get_value_type(target);
  return entry;
}

inline void request_popup(const std::string& target_key, popup_target_type target_type) {
  std::lock_guard lock{bind_mutex()};

  popup_target_key() = target_key;
  popup_target_type_value() = target_type;
  popup_open_requested() = true;
}

inline void close_popup_if_target(const std::string& target_key) {
  if (popup_target_key() != target_key) {
    return;
  }

  popup_target_key().clear();
  popup_label_target_key().clear();
  ImGui::CloseCurrentPopup();
}

inline void remove_indicator_bind(std::string target_key, popup_target_type target_type) {
  std::lock_guard lock{bind_mutex()};

  if (target_type == popup_target_type::value_bind) {
    auto& value_entries = entries();
    for (auto it = value_entries.begin(); it != value_entries.end();) {
      if (it->target_key == target_key) {
        it = value_entries.erase(it);
      } else {
        ++it;
      }
    }
    close_popup_if_target(target_key);
    return;
  }

  button_entry* entry = find_button_entry(target_key);
  if (entry == nullptr || entry->target == nullptr) {
    return;
  }

  entry->target->button = SDLK_UNKNOWN;
  entry->target->waiting = false;
  entry->target->active = false;
  entry->target->was_down = false;
  entry->target->last_press_time = 0.0f;
  entry->label = entry->default_label;

  close_popup_if_target(target_key);
}

template <typename value_t>
inline void maybe_open_popup(value_t* target, const char* label) {
  if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
    return;
  }

  bind_entry* entry = ensure_entry(target, label);
  if (entry == nullptr) {
    return;
  }

  request_popup(entry->target_key, popup_target_type::value_bind);
}

template <typename value_t>
inline void sync_default(value_t* target, bind_entry* entry, bool changed) {
  if (entry == nullptr || target == nullptr) {
    return;
  }

  if (!entry->has_default || changed) {
    entry->default_value = *target;
    entry->has_default = true;
  }

  if (!entry->has_override) {
    entry->override_value = *target;
    entry->has_override = true;
  }
}

inline void bindable_checkbox(const char* label, bool* target, bool changed) {
  std::lock_guard lock{bind_mutex()};

  bind_entry* entry = ensure_entry(target, label);
  if (entry != nullptr) {
    entry->widget = widget_type::checkbox;
    sync_default(target, entry, changed);
  }
  maybe_open_popup(target, label);
}

inline void bindable_combo_int(const char* label, int* target, bool changed, const char* const items[], int item_count) {
  std::lock_guard lock{bind_mutex()};

  bind_entry* entry = ensure_entry(target, label);
  if (entry != nullptr) {
    entry->widget = widget_type::combo_int;
    entry->options.clear();
    for (int i = 0; i < item_count; ++i) {
      entry->options.emplace_back(items[i], i);
    }
    sync_default(target, entry, changed);
  }
  maybe_open_popup(target, label);
}

inline void bindable_slider_int(const char* label, int* target, bool changed, int minimum, int maximum, const char* format) {
  std::lock_guard lock{bind_mutex()};

  bind_entry* entry = ensure_entry(target, label);
  if (entry != nullptr) {
    entry->widget = widget_type::slider_int;
    entry->int_min = minimum;
    entry->int_max = maximum;
    entry->format = format != nullptr ? format : "%d";
    sync_default(target, entry, changed);
  }
  maybe_open_popup(target, label);
}

inline void bindable_slider_float(const char* label, float* target, bool changed, float minimum, float maximum, const char* format) {
  std::lock_guard lock{bind_mutex()};

  bind_entry* entry = ensure_entry(target, label);
  if (entry != nullptr) {
    entry->widget = widget_type::slider_float;
    entry->float_min = minimum;
    entry->float_max = maximum;
    entry->format = format != nullptr ? format : "%.3f";
    sync_default(target, entry, changed);
  }
  maybe_open_popup(target, label);
}

inline void handle_input(SDL_Event* event) {
  std::lock_guard lock{bind_mutex()};

  register_builtin_targets();
  register_builtin_button_targets();

  for (bind_entry& entry : entries()) {
    const int previous_button = entry.trigger.button;
    const bool was_waiting = entry.trigger.waiting;
    ImGui::KeybindEvent(event, &entry.trigger.waiting, &entry.trigger.button);
    if (was_waiting && !entry.trigger.waiting && entry.trigger.button != previous_button) {
      entry.trigger.active = false;
      entry.trigger.was_down = false;
      entry.trigger.last_press_time = 0.0f;
    }
  }

  for (button_entry& entry : button_entries()) {
    if (entry.target == nullptr) {
      continue;
    }

    const int previous_button = entry.target->button;
    const bool was_waiting = entry.target->waiting;
    ImGui::KeybindEvent(event, &entry.target->waiting, &entry.target->button);
    if (was_waiting && !entry.target->waiting && entry.target->button != previous_button) {
      entry.target->active = false;
      entry.target->was_down = false;
      entry.target->last_press_time = 0.0f;
    }
  }
}

inline void run() {
  std::lock_guard lock{bind_mutex()};

  register_builtin_targets();
  register_builtin_button_targets();

  for (bind_entry& entry : entries()) {
    if (entry.target == nullptr) {
      const auto it = key_to_pointer().find(entry.target_key);
      if (it != key_to_pointer().end()) {
        entry.target = it->second;
      }
    }

    if (entry.target == nullptr || !entry.has_default || !entry.has_override) {
      continue;
    }

    const bool bind_set = entry.trigger.button != SDLK_UNKNOWN;
    if (!entry.enabled || !bind_set) {
      entry.active = false;
      continue;
    }

    entry.active = is_button_active(entry.trigger);
    write_value(entry.target, entry.type, entry.active ? entry.override_value : entry.default_value);
  }
}

inline void draw_popup() {
  std::lock_guard lock{bind_mutex()};

  if (popup_open_requested()) {
    ImGui::OpenPopup("bind_popup_context");
    popup_open_requested() = false;
  }

  ImGui::SetNextWindowSize(ImVec2(250.0f, 0.0f), ImGuiCond_Appearing);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(7.0f, 7.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.114f, 0.184f, 0.251f, 0.985f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.267f, 0.392f, 0.596f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.000f, 1.000f, 1.000f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.114f, 0.184f, 0.251f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.160f, 0.230f, 0.320f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.267f, 0.392f, 0.596f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.114f, 0.184f, 0.251f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.160f, 0.230f, 0.320f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.267f, 0.392f, 0.596f, 1.000f));
  ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.259f, 0.737f, 0.600f, 1.000f));

  if (!ImGui::BeginPopup("bind_popup_context")) {
    ImGui::PopStyleColor(10);
    ImGui::PopStyleVar(4);
    return;
  }

  const char* mode_names[] = { "Hold", "Toggle", "Double" };
  const bool popup_is_button = popup_target_type_value() == popup_target_type::button_bind;

  bind_entry* entry = popup_is_button ? nullptr : find_entry(popup_target_key());
  button_entry* button_entry = popup_is_button ? find_button_entry(popup_target_key()) : nullptr;
  if (entry == nullptr && button_entry == nullptr) {
    ImGui::TextUnformatted("Bind unavailable");
    ImGui::EndPopup();
    ImGui::PopStyleColor(10);
    ImGui::PopStyleVar(4);
    return;
  }

  button* popup_button = popup_is_button ? button_entry->target : &entry->trigger;
  std::string* popup_label = popup_is_button ? &button_entry->label : &entry->label;
  bool* popup_show_in_indicator = popup_is_button ? &button_entry->show_in_indicator : &entry->show_in_indicator;
  if (popup_button == nullptr || popup_label == nullptr || popup_show_in_indicator == nullptr) {
    ImGui::TextUnformatted("Bind unavailable");
    ImGui::EndPopup();
    ImGui::PopStyleColor(10);
    ImGui::PopStyleVar(4);
    return;
  }

  std::string key_label = "Not bound";
  if (popup_button != nullptr && popup_button->waiting) {
    key_label = "Press a key...";
  } else if (popup_button != nullptr && popup_button->button != SDLK_UNKNOWN) {
    key_label = get_button_name(popup_button->button);
  }

  if (popup_label_target_key() != popup_target_key()) {
    popup_label_target_key() = popup_target_key();
    popup_label_buffer().fill('\0');
    std::snprintf(popup_label_buffer().data(), popup_label_buffer().size(), "%s", popup_label->c_str());
  }

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.259f, 0.737f, 0.600f, 1.000f));
  ImGui::TextUnformatted("Bind editor");
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextDisabled("right-click feature");
  ImGui::Separator();

  ImGui::TextUnformatted(popup_label->c_str());
  ImGui::TextDisabled("Key");
  if (ImGui::Button(key_label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
    popup_button->waiting = true;
  }
  if (popup_button->waiting) {
    ImGui::TextDisabled("Press any key or mouse button. Escape clears it.");
  }

  if (popup_button->button != SDLK_UNKNOWN && ImGui::Button("Clear key", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
    popup_button->button = SDLK_UNKNOWN;
    reset_button_state(*popup_button);
  }

  if (ImGui::InputText("Name", popup_label_buffer().data(), popup_label_buffer().size())) {
    *popup_label = popup_label_buffer().data();
  }

  int mode = static_cast<int>(popup_button->mode);
  if (ImGui::Combo("Mode", &mode, mode_names, IM_ARRAYSIZE(mode_names))) {
    popup_button->mode = static_cast<button::mode_type>(mode);
    popup_button->active = false;
    popup_button->was_down = false;
    popup_button->last_press_time = 0.0f;
  }

  if (entry != nullptr && entry->type == value_type::boolean) {
    bool value = std::get<bool>(entry->override_value);
    const char* bool_names[] = { "Off", "On" };
    int bool_index = value ? 1 : 0;
    if (ImGui::Combo("Value", &bool_index, bool_names, IM_ARRAYSIZE(bool_names))) {
      entry->override_value = bool_index == 1;
    }
  } else if (entry != nullptr && entry->widget == widget_type::combo_int && !entry->options.empty()) {
    int value = std::get<int>(entry->override_value);
    std::vector<const char*> names{};
    names.reserve(entry->options.size());
    int current_index = 0;
    for (std::size_t i = 0; i < entry->options.size(); ++i) {
      names.push_back(entry->options[i].first.c_str());
      if (entry->options[i].second == value) {
        current_index = static_cast<int>(i);
      }
    }

    if (ImGui::Combo("Value", &current_index, names.data(), static_cast<int>(names.size()))) {
      entry->override_value = entry->options[static_cast<std::size_t>(current_index)].second;
    }
  } else if (entry != nullptr && entry->widget == widget_type::slider_int) {
    int value = std::get<int>(entry->override_value);
    if (ImGui::SliderInt("Value", &value, entry->int_min, entry->int_max, entry->format.c_str())) {
      entry->override_value = value;
    }
  } else if (entry != nullptr && entry->widget == widget_type::slider_float) {
    float value = std::get<float>(entry->override_value);
    if (ImGui::SliderFloat("Value", &value, entry->float_min, entry->float_max, entry->format.c_str())) {
      entry->override_value = value;
    }
  }

  if (entry != nullptr) {
    ImGui::Checkbox("Enabled", &entry->enabled);
  }
  ImGui::Checkbox("Show in indicator", popup_show_in_indicator);

  ImGui::Separator();
  if (ImGui::Button("Remove bind", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
    remove_indicator_bind(popup_target_key(), popup_target_type_value());
  }

  ImGui::EndPopup();
  ImGui::PopStyleColor(10);
  ImGui::PopStyleVar(4);
}

inline const std::vector<bind_entry>& indicator_entries() {
  return entries();
}

inline std::vector<indicator_row> collect_indicator_rows() {
  std::lock_guard lock{bind_mutex()};

  register_builtin_targets();
  register_builtin_button_targets();

  std::vector<indicator_row> rows{};
  rows.reserve(entries().size() + button_entries().size());

  for (const bind_entry& entry : entries()) {
    if (!entry.enabled || !entry.show_in_indicator || entry.trigger.button == SDLK_UNKNOWN) {
      continue;
    }

    rows.push_back({
      .label = entry.label,
      .key = get_button_name(entry.trigger.button),
      .state = entry.active ? "active" : "idle",
      .target_key = entry.target_key,
      .popup_type = popup_target_type::value_bind,
      .active = entry.active
    });
  }

  for (const button_entry& entry : button_entries()) {
    if (entry.target == nullptr || !entry.show_in_indicator || entry.target->button == SDLK_UNKNOWN) {
      continue;
    }

    bool show_row = false;
    bool active = false;
    const char* state = "idle";

    switch (entry.kind) {
    case indicator_kind::aimbot:
      show_row = true;
      active = entry.target->active;
      state = active ? "active" : "idle";
      break;
    case indicator_kind::value:
      break;
    }

    if (!show_row) {
      continue;
    }

    rows.push_back({
      .label = entry.label,
      .key = get_button_name(entry.target->button),
      .state = state,
      .target_key = entry.target_key,
      .popup_type = popup_target_type::button_bind,
      .active = active
    });
  }

  return rows;
}

inline void save_to_store(cathook::core::config_store* store) {
  std::lock_guard lock{bind_mutex()};

  if (store == nullptr) {
    return;
  }

  store->set_int("binds.count", static_cast<int>(entries().size()));
  for (int index = 0; index < static_cast<int>(entries().size()); ++index) {
    const bind_entry& entry = entries()[static_cast<std::size_t>(index)];
    const std::string prefix = "binds." + std::to_string(index) + ".";
    store->set_string(prefix + "target_key", entry.target_key);
    store->set_string(prefix + "label", entry.label);
    store->set_int(prefix + "type", static_cast<int>(entry.type));
    store->set_int(prefix + "widget", static_cast<int>(entry.widget));
    store->set_int(prefix + "button", entry.trigger.button);
    store->set_int(prefix + "mode", static_cast<int>(entry.trigger.mode));
    store->set_bool(prefix + "enabled", entry.enabled);
    store->set_bool(prefix + "show_in_indicator", entry.show_in_indicator);

    if (entry.type == value_type::boolean) {
      store->set_bool(prefix + "default_bool", std::get<bool>(entry.default_value));
      store->set_bool(prefix + "override_bool", std::get<bool>(entry.override_value));
    } else if (entry.type == value_type::integer) {
      store->set_int(prefix + "default_int", std::get<int>(entry.default_value));
      store->set_int(prefix + "override_int", std::get<int>(entry.override_value));
    } else if (entry.type == value_type::floating) {
      store->set_float(prefix + "default_float", std::get<float>(entry.default_value));
      store->set_float(prefix + "override_float", std::get<float>(entry.override_value));
    }
  }

  store->set_int("button_binds.count", static_cast<int>(button_entries().size()));
  for (int index = 0; index < static_cast<int>(button_entries().size()); ++index) {
    const button_entry& entry = button_entries()[static_cast<std::size_t>(index)];
    const std::string prefix = "button_binds." + std::to_string(index) + ".";
    store->set_string(prefix + "target_key", entry.target_key);
    store->set_string(prefix + "label", entry.label);
    store->set_bool(prefix + "show_in_indicator", entry.show_in_indicator);
  }
}

inline bool save(cathook::core::config_store* store, const std::string_view name) {
  if (store == nullptr) {
    return false;
  }

  cathook::core::config_store bind_store = store->scoped_store("configs/binds");
  save_to_store(&bind_store);
  return bind_store.save_file(name);
}

inline bool save(cathook::core::config_store* store) {
  if (store == nullptr) {
    return false;
  }

  return save(store, store->current_name());
}

inline void load_from_store(cathook::core::config_store* store) {
  std::lock_guard lock{bind_mutex()};

  if (store == nullptr) {
    return;
  }

  register_builtin_targets();
  register_builtin_button_targets();
  entries().clear();

  const int count = std::clamp(store->get_int("binds.count", 0), 0, static_cast<int>(key_to_pointer().size()));
  entries().reserve(static_cast<std::size_t>(count));
  for (int index = 0; index < count; ++index) {
    const std::string prefix = "binds." + std::to_string(index) + ".";
    bind_entry entry{};
    entry.target_key = store->get_string(prefix + "target_key", "");
    if (entry.target_key.empty()) {
      continue;
    }

    const auto target_it = key_to_pointer().find(entry.target_key);
    const auto type_it = key_to_type().find(entry.target_key);
    if (target_it == key_to_pointer().end() || type_it == key_to_type().end() || target_it->second == nullptr) {
      continue;
    }

    entry.target = target_it->second;
    entry.type = type_it->second;
    entry.label = store->get_string(prefix + "label", entry.target_key);
    entry.widget = static_cast<widget_type>(std::clamp(store->get_int(prefix + "widget", 0), 0, 3));
    entry.trigger.button = store->get_int(prefix + "button", SDLK_UNKNOWN);
    entry.trigger.mode = static_cast<button::mode_type>(std::clamp(store->get_int(prefix + "mode", 0), 0, 2));
    entry.enabled = store->get_bool(prefix + "enabled", true);
    entry.show_in_indicator = store->get_bool(prefix + "show_in_indicator", true);
    reset_button_state(entry.trigger);

    if (entry.type == value_type::boolean) {
      entry.default_value = store->get_bool(prefix + "default_bool", entry.target != nullptr ? *static_cast<bool*>(entry.target) : false);
      entry.override_value = store->get_bool(prefix + "override_bool", std::get<bool>(entry.default_value));
    } else if (entry.type == value_type::integer) {
      entry.default_value = store->get_int(prefix + "default_int", entry.target != nullptr ? *static_cast<int*>(entry.target) : 0);
      entry.override_value = store->get_int(prefix + "override_int", std::get<int>(entry.default_value));
    } else {
      entry.default_value = store->get_float(prefix + "default_float", entry.target != nullptr ? *static_cast<float*>(entry.target) : 0.0f);
      entry.override_value = store->get_float(prefix + "override_float", std::get<float>(entry.default_value));
    }

    entry.has_default = true;
    entry.has_override = true;
    entries().push_back(entry);
  }

  const int button_count = std::clamp(store->get_int("button_binds.count", 0), 0, static_cast<int>(button_entries().size()));
  for (int index = 0; index < button_count; ++index) {
    const std::string prefix = "button_binds." + std::to_string(index) + ".";
    const std::string target_key = store->get_string(prefix + "target_key", "");
    if (target_key.empty()) {
      continue;
    }

    button_entry* entry = find_button_entry(target_key);
    if (entry == nullptr) {
      continue;
    }

    entry->label = store->get_string(prefix + "label", entry->label);
    entry->show_in_indicator = store->get_bool(prefix + "show_in_indicator", entry->show_in_indicator);
  }
}

inline bool load(cathook::core::config_store* store) {
  if (store == nullptr) {
    return false;
  }

  cathook::core::config_store bind_store = store->scoped_store("configs/binds");
  if (bind_store.load_file(store->current_name())) {
    load_from_store(&bind_store);
    return true;
  }

  load_from_store(store);
  return false;
}

inline bool delete_file(cathook::core::config_store* store, const std::string_view name) {
  if (store == nullptr) {
    return false;
  }

  cathook::core::config_store bind_store = store->scoped_store("configs/binds");
  return bind_store.delete_file(name);
}

} // namespace cat_bind

#endif
