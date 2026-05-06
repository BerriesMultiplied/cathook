/*
/^-----^\   data: 2026-04-02
V  o o  V  file: src/core/detach.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

// TODO
// fucking broken!
#ifndef DETACH_HPP
#define DETACH_HPP

#include "print.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>

namespace cathook::core
{

inline std::atomic_bool detach_requested = false;
inline std::atomic_bool detach_started = false;
inline std::atomic<std::uint64_t> detach_request_time_ms = 0;

constexpr auto detach_grace_period = std::chrono::milliseconds(250);

bool unload_module_runtime();
bool is_runtime_detached();

inline void request_detach()
{
  const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  detach_request_time_ms.store(static_cast<std::uint64_t>(now_ms), std::memory_order_release);

  if (!detach_requested.exchange(true, std::memory_order_acq_rel)) {
    print("Detach queued\n");
  }
}

inline bool is_detach_pending()
{
  return detach_requested.load(std::memory_order_acquire) || detach_started.load(std::memory_order_acquire);
}

inline void service_detach_request()
{
  if (!detach_requested.load(std::memory_order_acquire)) {
    return;
  }

  const auto request_time_ms = detach_request_time_ms.load(std::memory_order_acquire);
  const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();

  if (request_time_ms != 0 && (now_ms - static_cast<std::int64_t>(request_time_ms)) < detach_grace_period.count()) {
    return;
  }

  if (detach_started.exchange(true, std::memory_order_acq_rel)) {
    return;
  }

  detach_requested.store(false, std::memory_order_release);

  if (!unload_module_runtime()) {
    print("Detach cleanup failed\n");
    detach_requested.store(false, std::memory_order_release);
    detach_started.store(false, std::memory_order_release);
  }
}

}

#endif
