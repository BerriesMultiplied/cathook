/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/games/tf2/sdk/interfaces/engine.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef ENGINE_HPP
#define ENGINE_HPP

class Player;
class KeyValues;
struct player_info;

#include "core/types.hpp"
#include "games/tf2/sdk/interfaces/achievement_mgr.hpp"

class Engine {
public:
  int get_localplayer_index(void) {
    void** vtable = *(void ***)this;

    int (*get_localplayer_index_fn)(void*) = (int (*)(void*))vtable[12];
    
    return get_localplayer_index_fn(this);
  }
  
  Vec2 get_screen_size(void) {
    void** vtable = *(void ***)this;

    void (*get_screen_size_fn)(void*, int*, int*) = (void (*)(void*, int*, int*))vtable[5];
    
    int width;
    int height;
    
    get_screen_size_fn(this, &width, &height);

    return Vec2{width, height};
  }

  bool is_in_game(void) {
    void** vtable = *(void ***)this;

    bool (*is_in_game_fn)(void*) = (bool (*)(void*))vtable[26];

    return is_in_game_fn(this);
  }

  bool is_connected(void) {
    void** vtable = *(void ***)this;

    bool (*is_connected_fn)(void*) = (bool (*)(void*))vtable[27];

    return is_connected_fn(this);
  }

  bool is_drawing_loading_image(void) {
    void** vtable = *(void ***)this;

    bool (*is_drawing_loading_image_fn)(void*) = (bool (*)(void*))vtable[28];

    return is_drawing_loading_image_fn(this);
  }

  void get_view_angles(Vec3& angles) {
    void** vtable = *(void ***)this;

    void (*get_view_angles_fn)(void*, Vec3&) = (void (*)(void*, Vec3&))vtable[19];

    get_view_angles_fn(this, angles);
  }

  void set_view_angles(Vec3& angles) {
    void** vtable = *(void ***)this;

    void (*set_view_angles_fn)(void*, Vec3&) = (void (*)(void*, Vec3&))vtable[20];

    set_view_angles_fn(this, angles);
  }

  const char* get_level_name(void) {
    void** vtable = *(void ***)this;

    const char* (*get_level_name_fn)(void*) = (const char* (*)(void*))vtable[51];

    return get_level_name_fn(this);
  }

  bool get_player_info(int entity_index, player_info* pinfo) {
    void** vtable = *(void ***)this;

    bool (*get_player_info_fn)(void*, int, player_info*) = (bool (*)(void*, int, player_info*))vtable[8];

    return get_player_info_fn(this, entity_index, pinfo);
  }

  int get_player_index_from_id(int user_id) {
    void** vtable = *(void ***)this;

    int (*get_player_index_from_id_fn)(void*, int) = (int (*)(void*, int))vtable[9];

    return get_player_index_from_id_fn(this, user_id);
  }

  void client_cmd_unrestricted(const char* cmd) {
    void** vtable = *(void ***)this;

    void (*client_cmd_unrestricted_fn)(void*, const char*) = (void (*)(void*, const char*))vtable[106];

    client_cmd_unrestricted_fn(this, cmd);
  }

  achievement_manager* get_achievement_manager() {
    void** vtable = *(void ***)this;

    auto get_achievement_manager_fn = reinterpret_cast<achievement_manager* (*)(void*)>(vtable[114]);

    return get_achievement_manager_fn(this);
  }

  void server_cmd_keyvalues(KeyValues* key_values) {
    void** vtable = *(void***)this;

    auto server_cmd_keyvalues_fn = (void (*)(void*, KeyValues*))vtable[127];

    server_cmd_keyvalues_fn(this, key_values);
  }
};

static inline Engine* engine;


#endif
