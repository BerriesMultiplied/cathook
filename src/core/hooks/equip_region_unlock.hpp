/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/equip_region_unlock.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef EQUIP_REGION_UNLOCK_HPP
#define EQUIP_REGION_UNLOCK_HPP

#include <cstdint>

inline std::uintptr_t (*item_schema_lookup_map_original)() = nullptr;
inline std::uintptr_t (*item_definition_lookup_original)(std::uintptr_t, unsigned int) = nullptr;

std::uintptr_t item_definition_lookup_hook(std::uintptr_t lookup_map, unsigned int item_index);

#endif
