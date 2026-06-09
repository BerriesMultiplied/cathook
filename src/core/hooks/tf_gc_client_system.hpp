/*
/^-----^\   data: 2026-05-08
V  o o  V  file: src/core/hooks/tf_gc_client_system.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef TF_GC_CLIENT_SYSTEM_HOOK_HPP
#define TF_GC_CLIENT_SYSTEM_HOOK_HPP

#include <cstdint>

inline std::intptr_t (*tf_gc_client_system_so_event_original)(void* self, void* shared_object, int event_type) = nullptr;
inline void (*tf_gc_client_system_request_accept_match_invite)(void* self, std::uint64_t lobby_id) = nullptr;

std::intptr_t tf_gc_client_system_so_event_hook(void* self, void* shared_object, int event_type);

#endif
