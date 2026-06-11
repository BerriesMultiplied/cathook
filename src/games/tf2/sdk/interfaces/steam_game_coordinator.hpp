#ifndef STEAM_GAME_COORDINATOR_HPP
#define STEAM_GAME_COORDINATOR_HPP

#include <cstdint>

class steam_game_coordinator
{
public:
  enum class result : int
  {
    ok = 0,
    no_message = 1,
    buffer_too_small = 2,
    not_logged_on = 3,
    invalid_message = 4,
  };

  bool send_message(std::uint32_t message_type, const void* data, std::uint32_t data_size)
  {
    void** vtable = *reinterpret_cast<void***>(this);
    int (*send_message_fn)(void*, std::uint32_t, const void*, std::uint32_t) =
      reinterpret_cast<int (*)(void*, std::uint32_t, const void*, std::uint32_t)>(vtable[0]);
    return send_message_fn(this, message_type, data, data_size) == static_cast<int>(result::ok);
  }

  result is_message_available(std::uint32_t* data_size)
  {
    void** vtable = *reinterpret_cast<void***>(this);
    auto is_message_available_fn = reinterpret_cast<int (*)(void*, std::uint32_t*)>(vtable[1]);
    return static_cast<result>(is_message_available_fn(this, data_size));
  }

  result retrieve_message(std::uint32_t* message_type, void* data, std::uint32_t data_size, std::uint32_t* actual_data_size)
  {
    void** vtable = *reinterpret_cast<void***>(this);
    auto retrieve_message_fn =
      reinterpret_cast<int (*)(void*, std::uint32_t*, void*, std::uint32_t, std::uint32_t*)>(vtable[2]);
    return static_cast<result>(retrieve_message_fn(this, message_type, data, data_size, actual_data_size));
  }
};

inline static steam_game_coordinator* steam_game_coordinator_interface;

#endif
