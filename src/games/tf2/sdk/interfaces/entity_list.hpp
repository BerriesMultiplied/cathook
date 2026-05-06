/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/interfaces/entity_list.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef PLAYER_LIST_HPP
#define PLAYER_LIST_HPP

#include "games/tf2/sdk/interfaces/engine.hpp"

class Entity;
class Player;

class EntityList {
public:
  Entity* entity_from_index(unsigned int index) {
    void** vtable = *(void ***)this;
     
    Entity* (*entity_from_index_fn)(void*, unsigned int) = (Entity* (*)(void*, unsigned int))vtable[3];
    
    return entity_from_index_fn(this, index);
  }

  Player* player_from_index(unsigned int index) {
    return (Player*)this->entity_from_index(index);
  }

  Entity* entity_from_handle(int handle) {
    if (handle == 0 || handle == -1) {
      return nullptr;
    }

    constexpr int max_entity_entries = 8192;
    constexpr int entity_entry_mask = max_entity_entries - 1;

    auto entity_index = handle & entity_entry_mask;
    if (entity_index <= 0 || entity_index == entity_entry_mask) {
      return nullptr;
    }

    auto max_entities = get_max_entities();
    if (entity_index >= max_entities) {
      return nullptr;
    }

    return entity_from_index(entity_index);
  }
  
  Player* get_localplayer(void) {
    return this->player_from_index(engine->get_localplayer_index());
  }
  
  Entity* get_entity_from_id(int user_id) {
    return this->entity_from_index(engine->get_player_index_from_id(user_id));
  }

  Player* get_player_from_id(int user_id) {
    return this->player_from_index(engine->get_player_index_from_id(user_id));
  }

  
  int get_max_entities(void) {
    void** vtable = *(void ***)this;
    
    int (*get_max_entities_fn)(void*) = (int (*)(void*))vtable[8];

    return get_max_entities_fn(this);
  }
};

static inline EntityList* entity_list;

#endif
