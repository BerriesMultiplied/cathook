/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/automation/navbot/navbot_hazards.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/automation/navbot/navbot_hazards.hpp"

#include <algorithm>

namespace navbot
{

namespace
{

bool hazard_affects_path_validity(const hazard_record& record)
{
  return record.policy != hazard_policy::soft_cost;
}

} // namespace

void navbot_hazards::clear()
{
  records_.clear();
  ++generation_;
}

void navbot_hazards::update_expired(float current_time)
{
  auto old_size = records_.size();
  auto removed_path_validity_hazard = false;

  records_.erase(
    std::remove_if(records_.begin(), records_.end(), [current_time, &removed_path_validity_hazard](const hazard_record& record)
    {
      const auto expired = record.expire_time > 0.0f && record.expire_time <= current_time;
      if (expired && hazard_affects_path_validity(record))
      {
        removed_path_validity_hazard = true;
      }

      return expired;
    }),
    records_.end());

  if (records_.size() != old_size && removed_path_validity_hazard)
  {
    ++generation_;
  }
}

void navbot_hazards::add_area_hazard(const hazard_record& record)
{
  records_.push_back(record);
  if (hazard_affects_path_validity(record))
  {
    ++generation_;
  }
}

void navbot_hazards::add_edge_hazard(const hazard_record& record)
{
  records_.push_back(record);
  if (hazard_affects_path_validity(record))
  {
    ++generation_;
  }
}

void navbot_hazards::add_transition_failure(nav_edge_id edge_id, float current_time, float duration)
{
  hazard_record record{};
  record.kind = hazard_kind::transition_failure;
  record.policy = hazard_policy::temporary_forbid;
  record.edge_id = edge_id;
  record.expire_time = current_time + duration;
  add_edge_hazard(record);
}

void navbot_hazards::add_crumb_blacklist(nav_area_id area_id, nav_edge_id edge_id, float current_time, float duration)
{
  if (duration <= 0.0f)
  {
    return;
  }

  auto added_record = false;
  const auto expire_time = current_time + duration;

  if (area_id.valid())
  {
    hazard_record record{};
    record.kind = hazard_kind::crumb_blacklist;
    record.policy = hazard_policy::temporary_forbid;
    record.area_id = area_id;
    record.expire_time = expire_time;
    records_.push_back(record);
    added_record = true;
  }

  if (edge_id.from_area != 0)
  {
    hazard_record record{};
    record.kind = hazard_kind::crumb_blacklist;
    record.policy = hazard_policy::temporary_forbid;
    record.edge_id = edge_id;
    record.expire_time = expire_time;
    records_.push_back(record);
    added_record = true;
  }

  if (added_record)
  {
    ++generation_;
  }
}

bool navbot_hazards::is_area_blocked(nav_area_id area_id, float current_time) const
{
  for (const auto& record : records_)
  {
    if (record.area_id.value != area_id.value)
    {
      continue;
    }

    if (record.expire_time > 0.0f && record.expire_time <= current_time)
    {
      continue;
    }

    if (record.policy == hazard_policy::hard_block || record.policy == hazard_policy::temporary_forbid)
    {
      return true;
    }
  }

  return false;
}

bool navbot_hazards::is_edge_blocked(nav_edge_id edge_id, float current_time) const
{
  for (const auto& record : records_)
  {
    if (record.edge_id.from_area != edge_id.from_area || record.edge_id.connection_index != edge_id.connection_index)
    {
      continue;
    }

    if (record.expire_time > 0.0f && record.expire_time <= current_time)
    {
      continue;
    }

    if (record.policy == hazard_policy::temporary_forbid || record.policy == hazard_policy::hard_block)
    {
      return true;
    }
  }

  return false;
}

float navbot_hazards::area_cost(nav_area_id area_id, float current_time) const
{
  auto total_cost = 0.0f;

  for (const auto& record : records_)
  {
    if (record.area_id.value != area_id.value)
    {
      continue;
    }

    if (record.expire_time > 0.0f && record.expire_time <= current_time)
    {
      continue;
    }

    if (record.policy == hazard_policy::soft_cost)
    {
      total_cost += record.cost;
    }
  }

  return total_cost;
}

bool navbot_hazards::has_active_world_hazard(float current_time) const
{
  for (const auto& record : records_)
  {
    if (record.expire_time > 0.0f && record.expire_time <= current_time)
    {
      continue;
    }

    if (record.kind == hazard_kind::transition_failure)
    {
      continue;
    }

    return true;
  }

  return false;
}

uint32_t navbot_hazards::generation() const
{
  return generation_;
}

const std::vector<hazard_record>& navbot_hazards::records() const
{
  return records_;
}

} // namespace navbot
