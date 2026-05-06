/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/interfaces/steam_client.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef STEAM_CLIENT_HPP
#define STEAM_CLIENT_HPP

#include "games/tf2/sdk/interfaces/steam_user_stats.hpp"

class SteamClient {
public:
  /* Steam Pipe Handle */ int create_steam_pipe(void) {
    void** vtable = *(void***)this;

    int (*create_steam_pipe_fn)(void*) = (int (*)(void*))vtable[0];
    
    return create_steam_pipe_fn(this);
  }

  /* Steam User Handle */ int connect_to_global_user(int steam_pipe) {
    void** vtable = *(void***)this;

    int (*connect_to_global_user_fn)(void*, int) = (int (*)(void*, int))vtable[2];

    return connect_to_global_user_fn(this, steam_pipe);
  }

  /* Steam Friends Interface */ void* get_steam_friends_interface(int steam_user, int steam_pipe, const char* version) {
    void** vtable = *(void***)this;

    void* (*get_steam_friends_interface_fn)(void*, int, int, const char*) = (void* (*)(void*, int, int, const char*))vtable[8];

    return get_steam_friends_interface_fn(this, steam_user, steam_pipe, version);
  }

  steam_user_stats* get_steam_user_stats_interface(int steam_user, int steam_pipe, const char* version) {
    void** vtable = *(void***)this;

    auto get_steam_user_stats_interface_fn = reinterpret_cast<steam_user_stats* (*)(void*, int, int, const char*)>(vtable[13]);

    return get_steam_user_stats_interface_fn(this, steam_user, steam_pipe, version);
  }
};

inline static SteamClient* steam_client;

#endif 
