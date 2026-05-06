/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/cl_read_packets.hpp
 |  Y  |   autor: pupnoodle
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

void run_network_fix_before_move(bool final_tick);

#endif
