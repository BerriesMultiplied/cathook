/*
/^-----^\   data: 2026-05-09
V  o o  V  file: src/games/tf2/sdk/interfaces/mdl_cache.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef MDL_CACHE_HPP
#define MDL_CACHE_HPP

class mdl_cache_interface {
public:
  void begin_lock() {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto begin_lock_fn = reinterpret_cast<void (*)(void*)>(vtable[25]);
    begin_lock_fn(this);
  }

  void end_lock() {
    auto** vtable = *reinterpret_cast<void***>(this);
    auto end_lock_fn = reinterpret_cast<void (*)(void*)>(vtable[26]);
    end_lock_fn(this);
  }
};

inline static mdl_cache_interface* mdl_cache;

#endif
