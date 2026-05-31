/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/cl_read_packets.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef CL_READ_PACKETS_HPP
#define CL_READ_PACKETS_HPP

#include <cstdint>

extern std::int64_t (*cl_read_packets_original)(char);

namespace network_fix
{
[[nodiscard]] bool is_active();
[[nodiscard]] int adjusted_tick_count(int default_tick);
[[nodiscard]] float adjusted_curtime(float default_curtime);
}

void run_network_fix_before_move(bool final_tick);

#endif
