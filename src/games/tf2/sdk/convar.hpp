/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/convar.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef CONVAR_HPP
#define CONVAR_HPP

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstddef>

class Convar {
public:
  void set_int(int value) {
    auto* target = value_target();
    *field<int>(target, int_value_offset) = value;
    *field<float>(target, float_value_offset) = static_cast<float>(value);
    set_string_value(target, value);
  }

  int get_int() {
    return *field<int>(value_target(), int_value_offset);
  }

  const char* get_string() {
    auto* string_value = *field<char*>(value_target(), string_value_offset);
    return string_value != nullptr ? string_value : "";
  }

  void set_float(float value) {
    auto* target = value_target();
    *field<float>(target, float_value_offset) = value;
    *field<int>(target, int_value_offset) = static_cast<int>(value);
  }

  float get_float() {
    return *field<float>(value_target(), float_value_offset);
  }

private:
  static constexpr std::size_t parent_offset = 0x38;
  static constexpr std::size_t string_value_offset = 0x48;
  static constexpr std::size_t string_length_offset = 0x50;
  static constexpr std::size_t float_value_offset = 0x54;
  static constexpr std::size_t int_value_offset = 0x58;

  template <typename value_type>
  static value_type* field(Convar* convar, std::size_t offset) {
    return reinterpret_cast<value_type*>(reinterpret_cast<std::uint8_t*>(convar) + offset);
  }

  Convar* value_target() {
    auto* parent = *field<Convar*>(this, parent_offset);
    return parent != nullptr ? parent : this;
  }

  static void set_string_value(Convar* convar, int value) {
    auto* string_value = *field<char*>(convar, string_value_offset);
    const auto string_length = *field<int>(convar, string_length_offset);
    if (string_value == nullptr || string_length <= 0) {
      return;
    }

    char value_buffer[32]{};
    const int written = std::snprintf(value_buffer, sizeof(value_buffer), "%d", value);
    if (written < 0 || written + 1 > string_length) {
      return;
    }

    std::memcpy(string_value, value_buffer, static_cast<std::size_t>(written + 1));
  }
};

#endif
