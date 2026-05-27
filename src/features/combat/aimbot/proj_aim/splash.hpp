#ifndef SPLASH_HPP
#define SPLASH_HPP

#include "features/combat/aimbot/proj_aim/proj_aim_splash.hpp"

namespace splash {

using point = proj_aim_splash_point;
using history = proj_aim_splash_history;

inline aimbot_candidate find_candidate(Player* localplayer,
  Weapon* weapon,
  Player* player,
  user_cmd* cmd,
  const Vec3& original_view_angles,
  const LocalPredictionEntityPath& target_path) {
  return proj_aim_find_splash_candidate(localplayer, weapon, player, cmd, original_view_angles, target_path);
}

}

#endif
