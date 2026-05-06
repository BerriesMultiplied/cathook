/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/memory/byte_patch.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef BYTE_PATCH_HPP
#define BYTE_PATCH_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

#include "core/print.hpp"

class byte_patch
{
public:
  byte_patch() = default;

  byte_patch(void* target, std::initializer_list<std::uint8_t> patch_bytes)
      : target_(reinterpret_cast<std::uint8_t*>(target)),
        patch_bytes_(patch_bytes)
  {
  }

  [[nodiscard]] bool valid() const
  {
    return target_ != nullptr && !patch_bytes_.empty();
  }

  bool apply()
  {
    if (!valid())
    {
      return false;
    }

    if (applied_)
    {
      return true;
    }

    original_bytes_.resize(patch_bytes_.size());
    std::memcpy(original_bytes_.data(), target_, original_bytes_.size());

    if (!set_page_protection(PROT_READ | PROT_WRITE | PROT_EXEC))
    {
      return false;
    }

    std::memcpy(target_, patch_bytes_.data(), patch_bytes_.size());
    __builtin___clear_cache(reinterpret_cast<char*>(target_), reinterpret_cast<char*>(target_ + patch_bytes_.size()));
    set_page_protection(PROT_READ | PROT_EXEC);
    applied_ = true;
    return true;
  }

  bool restore()
  {
    if (!applied_ || target_ == nullptr || original_bytes_.empty())
    {
      return true;
    }

    if (!set_page_protection(PROT_READ | PROT_WRITE | PROT_EXEC))
    {
      return false;
    }

    std::memcpy(target_, original_bytes_.data(), original_bytes_.size());
    __builtin___clear_cache(reinterpret_cast<char*>(target_), reinterpret_cast<char*>(target_ + original_bytes_.size()));
    set_page_protection(PROT_READ | PROT_EXEC);
    applied_ = false;
    return true;
  }

private:
  bool set_page_protection(int protection) const
  {
    const auto page_size = static_cast<std::uintptr_t>(sysconf(_SC_PAGESIZE));
    auto* page = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(target_) & ~(page_size - 1));
    if (mprotect(page, page_size, protection) != 0)
    {
      print("[nographics] mprotect failed for %p\n", target_);
      return false;
    }

    return true;
  }

  std::uint8_t* target_ = nullptr;
  std::vector<std::uint8_t> patch_bytes_{};
  std::vector<std::uint8_t> original_bytes_{};
  bool applied_ = false;
};

#endif
