/*
/^-----^\   data: 2026-05-06
V  o o  V  file: src/games/tf2/sdk/interfaces/steam_runtime.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/


// this is fucking horrid.

#ifndef STEAM_RUNTIME_HPP
#define STEAM_RUNTIME_HPP

#include <array>
#include <dlfcn.h>

#include "games/tf2/sdk/interfaces/steam_client.hpp"
#include "games/tf2/sdk/interfaces/steam_user_stats.hpp"

void* get_interface(const char* lib_path, const char* version);
void* open_loaded_library(const char* lib_path);

namespace steam_runtime
{

template <typename function_type>
function_type resolve_loaded_symbol(const char* symbol_name)
{
  if (auto* symbol = dlsym(RTLD_DEFAULT, symbol_name))
  {
    return reinterpret_cast<function_type>(symbol);
  }

  constexpr std::array steam_api_modules = {
    "./bin/linux64/libsteam_api.so",
    "libsteam_api.so",
  };

  for (const char* module_name : steam_api_modules)
  {
    void* handle = open_loaded_library(module_name);
    if (handle == nullptr)
    {
      continue;
    }

    auto* symbol = dlsym(handle, symbol_name);
    dlclose(handle);

    if (symbol != nullptr)
    {
      return reinterpret_cast<function_type>(symbol);
    }
  }

  return nullptr;
}

inline SteamClient* resolve_steam_client_from_api()
{
  using steam_client_factory_fn = SteamClient* (*)();

  auto* factory = resolve_loaded_symbol<steam_client_factory_fn>("SteamClient");
  return factory != nullptr ? factory() : nullptr;
}

inline SteamClient* resolve_steam_client()
{
  if (steam_client != nullptr)
  {
    return steam_client;
  }

  if (auto* client = resolve_steam_client_from_api())
  {
    steam_client = client;
    return steam_client;
  }

  constexpr std::array steam_client_modules = {
    "./bin/linux64/steamclient.so",
    "steamclient.so",
    "./bin/linux64/libsteam_api.so",
    "libsteam_api.so",
  };

  constexpr std::array steam_client_versions = {
    "SteamClient020",
    "SteamClient017",
  };

  for (const char* module_name : steam_client_modules)
  {
    for (const char* version : steam_client_versions)
    {
      auto* candidate = static_cast<SteamClient*>(get_interface(module_name, version));
      if (candidate != nullptr)
      {
        steam_client = candidate;
        return steam_client;
      }
    }
  }

  return nullptr;
}

inline steam_user_stats* resolve_steam_user_stats_from_api()
{
  using steam_user_stats_factory_fn = steam_user_stats* (*)();

  constexpr std::array factory_names = {
    "SteamAPI_SteamUserStats_v012",
    "SteamAPI_SteamUserStats_v011",
  };

  for (const char* factory_name : factory_names)
  {
    auto* factory = resolve_loaded_symbol<steam_user_stats_factory_fn>(factory_name);
    if (factory == nullptr)
    {
      continue;
    }

    if (auto* stats = factory())
    {
      return stats;
    }
  }

  return nullptr;
}

inline steam_user_stats* resolve_steam_user_stats()
{
  if (steam_user_stats_interface != nullptr)
  {
    return steam_user_stats_interface;
  }

  if (auto* stats = resolve_steam_user_stats_from_api())
  {
    steam_user_stats_interface = stats;
    return steam_user_stats_interface;
  }

  auto* steam_client_interface = resolve_steam_client();
  if (steam_client_interface == nullptr)
  {
    return nullptr;
  }

  static int steam_pipe = 0;
  static int steam_user = 0;

  if (steam_pipe == 0)
  {
    steam_pipe = steam_client_interface->create_steam_pipe();
  }

  if (steam_pipe == 0)
  {
    return nullptr;
  }

  if (steam_user == 0)
  {
    steam_user = steam_client_interface->connect_to_global_user(steam_pipe);
  }

  if (steam_user == 0)
  {
    return nullptr;
  }

  constexpr std::array user_stats_versions = {
    "STEAMUSERSTATS_INTERFACE_VERSION012",
    "STEAMUSERSTATS_INTERFACE_VERSION011",
  };

  for (const char* version : user_stats_versions)
  {
    steam_user_stats_interface = steam_client_interface->get_steam_user_stats_interface(steam_user, steam_pipe, version);
    if (steam_user_stats_interface != nullptr)
    {
      return steam_user_stats_interface;
    }
  }

  return nullptr;
}

} // namespace steam_runtime

#endif
