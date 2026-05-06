/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/games/tf2/sdk/interfaces/file_system.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef FILE_SYSTEM_HPP
#define FILE_SYSTEM_HPP

#include <cstdint>

using file_handle_t = void*;
using file_find_handle_t = void*;
using file_cache_handle_t = void*;

class file_system
{
};

inline static file_system* game_file_system;

#endif
