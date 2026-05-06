/*
/^-----^\   data: 2026-05-01
V  o o  V  file: src/core/ipc/ipc_client.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef CAT_IPC_CLIENT_HPP
#define CAT_IPC_CLIENT_HPP

#include <cstdint>

namespace cat_ipc::client
{

void set_enabled(bool enabled);
void set_auto_ignore_enabled(bool enabled);
void start();
void tick();
void shutdown();
[[nodiscard]] bool connected();
[[nodiscard]] int peer_id();
[[nodiscard]] bool is_local_ipc_friend(std::uint32_t friend_id);

} // namespace cat_ipc::client

#endif
