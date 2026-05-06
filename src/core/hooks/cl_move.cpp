/*
/^-----^\   data: 2026-05-01
V  o o  V  file: src/core/hooks/cl_move.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include <cstdint>

#include "features/combat/tickbase/tickbase.hpp"
#include "games/tf2/sdk/entities/player.hpp"
#include "games/tf2/sdk/interfaces/client.hpp"

tickbase::cl_move_fn cl_move_original = nullptr;

using prediction_run_simulation_fn = void (*)(void*, int, user_cmd*, Player*, float);

prediction_run_simulation_fn prediction_run_simulation_original = nullptr;

namespace
{

auto resolve_rip_relative(std::uint8_t* instruction, int displacement_offset, int instruction_size) -> std::uint8_t*
{
  const auto displacement = *reinterpret_cast<std::int32_t*>(instruction + displacement_offset);
  return instruction + instruction_size + displacement;
}

auto resolve_cl_move_lea(void* cl_move, int offset) -> std::uint8_t*
{
  auto* instruction = reinterpret_cast<std::uint8_t*>(cl_move) + offset;
  if (instruction[0] != 0x48 || instruction[1] != 0x8D || instruction[2] != 0x05) {
    return nullptr;
  }

  return resolve_rip_relative(instruction, 3, 7);
}

} // namespace

void cl_move_hook(float accumulated_extra_samples, bool final_tick)
{
  tickbase::move(accumulated_extra_samples, final_tick, cl_move_original);
}

void prediction_run_simulation_hook(void* prediction_instance, int current_command, user_cmd* cmd, Player* localplayer,
  float curtime)
{
  tickbase::apply_prediction_fix(current_command, cmd, localplayer, &curtime);
  prediction_run_simulation_original(prediction_instance, current_command, cmd, localplayer, curtime);
}

void initialize_cl_move_globals(tickbase::host_should_run_fn host_should_run)
{
  if (cl_move_original == nullptr) {
    return;
  }

  constexpr int net_time_lea_offset = 0x16f;
  constexpr int host_frametime_unbounded_lea_offset = 0x45d;
  constexpr int host_frametime_std_deviation_lea_offset = 0x48d;

  auto* net_time = reinterpret_cast<double*>(resolve_cl_move_lea(reinterpret_cast<void*>(cl_move_original), net_time_lea_offset));
  auto* host_frametime_unbounded =
    reinterpret_cast<float*>(resolve_cl_move_lea(reinterpret_cast<void*>(cl_move_original), host_frametime_unbounded_lea_offset));
  auto* host_frametime_std_deviation =
    reinterpret_cast<float*>(resolve_cl_move_lea(reinterpret_cast<void*>(cl_move_original), host_frametime_std_deviation_lea_offset));

  tickbase::initialize_engine_globals(net_time, host_frametime_unbounded, host_frametime_std_deviation, host_should_run);
}
