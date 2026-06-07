#ifndef VISUAL_GROUPS_HPP
#define VISUAL_GROUPS_HPP

#include "features/menu/config.hpp"

#include <memory>

class Entity;
class Player;

namespace visual_groups
{

struct visual_group_snapshot;

struct visual_group_match
{
  std::shared_ptr<const visual_group_snapshot> snapshot{};
  const visual_group* group = nullptr;

  [[nodiscard]] explicit operator bool() const
  {
    return group != nullptr;
  }

  [[nodiscard]] const visual_group& operator*() const
  {
    return *group;
  }

  [[nodiscard]] const visual_group* operator->() const
  {
    return group;
  }
};

void ensure_defaults();
void store(Player* localplayer);
[[nodiscard]] visual_group_match group_for_entity(Entity* entity, bool models = true);
[[nodiscard]] bool groups_active();
void move_group(int from, int to);
[[nodiscard]] RGBA_float color_for_entity(Entity* entity, const visual_group& group);
[[nodiscard]] float alpha_for_entity(Entity* entity, float start, float end, bool smooth_alpha);
[[nodiscard]] const char* label_for_entity(Entity* entity);

} 

#endif
