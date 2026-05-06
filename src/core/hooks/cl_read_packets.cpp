/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/cl_read_packets.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "cl_read_packets.hpp"

#include "features/menu/config.hpp"

#include "games/tf2/sdk/interfaces/client_state.hpp"
#include "games/tf2/sdk/interfaces/engine.hpp"
#include "games/tf2/sdk/interfaces/global_vars.hpp"

std::int64_t (*cl_read_packets_original)(char) = nullptr;

namespace
{

struct read_packet_state
{
  float client_state_frame_time = 0.0f;
  float frame_time = 0.0f;
  float current_time = 0.0f;
  int tick_count = 0;

  void store()
  {
    if (client_state == nullptr || global_vars == nullptr)
    {
      return;
    }

    client_state_frame_time = client_state->m_frameTime;
    frame_time = global_vars->frametime;
    current_time = global_vars->curtime;
    tick_count = global_vars->tickcount;
  }

  void restore() const
  {
    if (client_state == nullptr || global_vars == nullptr)
    {
      return;
    }

    client_state->m_frameTime = client_state_frame_time;
    global_vars->frametime = frame_time;
    global_vars->curtime = current_time;
    global_vars->tickcount = tick_count;
  }
};

read_packet_state g_read_packets_state{};
bool g_has_read_packets_state = false;

bool should_run_network_fix()
{
  return config.misc.exploits.network_fix &&
         engine != nullptr &&
         client_state != nullptr &&
         global_vars != nullptr &&
         engine->is_in_game() &&
         client_state->m_NetChannel != nullptr;
}

} // namespace

void run_network_fix_before_move(bool final_tick)
{
  if (!should_run_network_fix() || cl_read_packets_original == nullptr)
  {
    g_has_read_packets_state = false;
    return;
  }

  read_packet_state backup_state{};
  backup_state.store();
  cl_read_packets_original(final_tick ? 1 : 0);
  g_read_packets_state.store();
  backup_state.restore();
  g_has_read_packets_state = true;
}

std::int64_t cl_read_packets_hook(char final_tick)
{
  if (should_run_network_fix() && g_has_read_packets_state)
  {
    g_read_packets_state.restore();
    g_has_read_packets_state = false;
    return 0;
  }

  return cl_read_packets_original(final_tick);
}
