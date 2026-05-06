/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/equip_region_unlock.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "equip_region_unlock.hpp"

#include <cstddef>

#include "features/menu/config.hpp"

namespace
{

struct item_definition_masks
{
  std::byte pad[0xB0];
  std::uint64_t equip_region_mask = 0;
  std::uint64_t equip_region_conflict_mask = 0;
};

auto should_unlock_equip_regions(std::uintptr_t lookup_map) -> bool
{
  if (!config.misc.exploits.equip_region_unlock ||
      item_schema_lookup_map_original == nullptr)
  {
    return false;
  }

  return lookup_map == item_schema_lookup_map_original();
}

} // namespace

std::uintptr_t item_definition_lookup_hook(std::uintptr_t lookup_map, unsigned int item_index)
{
  if (item_definition_lookup_original == nullptr)
  {
    return 0;
  }

  const std::uintptr_t item_definition = item_definition_lookup_original(lookup_map, item_index);
  if (!should_unlock_equip_regions(lookup_map) || item_definition == 0)
  {
    return item_definition;
  }

  auto* definition = reinterpret_cast<item_definition_masks*>(item_definition);
  definition->equip_region_mask = 0;
  definition->equip_region_conflict_mask = 0;
  return item_definition;
}
