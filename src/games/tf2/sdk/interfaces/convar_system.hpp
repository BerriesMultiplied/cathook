/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/interfaces/convar_system.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef CONVAR_SYSTEM_HPP
#define CONVAR_SYSTEM_HPP

#include "games/tf2/sdk/convar.hpp"

class ConvarSystem {
public:
  void register_command(void* command_base) {
    void** vtable = *(void ***)this;
    auto register_command_fn = (void (*)(void*, void*))vtable[6];
    register_command_fn(this, command_base);
  }

  void unregister_command(void* command_base) {
    void** vtable = *(void ***)this;
    auto unregister_command_fn = (void (*)(void*, void*))vtable[7];
    unregister_command_fn(this, command_base);
  }

  Convar* find_var(const char* var_name) {
    void** vtable = *(void ***)this;
    
    Convar* (*find_var_fn)(void*, const char*) = (Convar* (*)(void*, const char*))vtable[12];
  
    return find_var_fn(this, var_name);
  }

};

inline static ConvarSystem* convar_system;

#endif
