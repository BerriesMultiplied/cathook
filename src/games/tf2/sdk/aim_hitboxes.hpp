/*
/^-----^\   data: 2026-04-05
V  o o  V  file: src/games/tf2/sdk/aim_hitboxes.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef AIM_HITBOXES_HPP
#define AIM_HITBOXES_HPP

#include <cstdint>

enum aim_hitbox_id {
  aim_hitbox_head = 0,
  aim_hitbox_pelvis,
  aim_hitbox_spine_0,
  aim_hitbox_spine_1,
  aim_hitbox_spine_2,
  aim_hitbox_spine_3,
  aim_hitbox_left_upper_arm,
  aim_hitbox_left_forearm,
  aim_hitbox_left_hand,
  aim_hitbox_right_upper_arm,
  aim_hitbox_right_forearm,
  aim_hitbox_right_hand,
  aim_hitbox_left_thigh,
  aim_hitbox_left_calf,
  aim_hitbox_left_foot,
  aim_hitbox_right_thigh,
  aim_hitbox_right_calf,
  aim_hitbox_right_foot
};

enum aim_hitbox_mask : uint32_t {
  aim_hitbox_mask_none = 0,
  aim_hitbox_mask_head = 1u << 0,
  aim_hitbox_mask_body = 1u << 1,
  aim_hitbox_mask_pelvis = 1u << 2,
  aim_hitbox_mask_arms = 1u << 3,
  aim_hitbox_mask_legs = 1u << 4,
  aim_hitbox_mask_auto = 1u << 5,
  aim_hitbox_mask_default_hitscan = aim_hitbox_mask_head | aim_hitbox_mask_body | aim_hitbox_mask_pelvis,
  aim_hitbox_mask_default_melee = aim_hitbox_mask_body | aim_hitbox_mask_pelvis,
  aim_hitbox_mask_all = aim_hitbox_mask_head | aim_hitbox_mask_body | aim_hitbox_mask_pelvis | aim_hitbox_mask_arms | aim_hitbox_mask_legs
};

#endif
