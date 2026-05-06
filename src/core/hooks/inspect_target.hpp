/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/inspect_target.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef INSPECT_TARGET_HPP
#define INSPECT_TARGET_HPP

#include <cstdint>

inline std::int64_t (*inspect_target_check_original)(void*, void*) = nullptr;

std::int64_t inspect_target_check_hook(void* me, void* target);

#endif
