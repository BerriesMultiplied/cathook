/*
/^-----^\   data: 2026-04-05
V  o o  V  file: src/games/tf2/sdk/interfaces/model_info.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef MODEL_INFO_HPP
#define MODEL_INFO_HPP

#include "core/types.hpp"

struct model_t {
  void* handle;
  char* name;
  int load_flags;
  int server_count;
  int type;
  int flags;
  Vec3 vec_mins;
  Vec3 vec_maxs;
  float radius;
};

struct matrix_3x4 {
  float mat[3][4];
};

struct studio_box {
  int bone;
  int group;
  Vec3 bbmin;
  Vec3 bbmax;
  int name_index;
  Vec3 angle;
  float radius;
  unsigned char pad[0x10];
};

struct studio_hitbox_set {
  int name_index;
  int num_hitboxes;
  int hitbox_index;

  studio_box* hitbox(int index) const {
    return ((studio_box*)((unsigned char*)this + hitbox_index)) + index;
  }
};

struct studio_hdr {
  int id;
  int version;
  int checksum;
  char name[64];
  int length;
  Vec3 eye_position;
  Vec3 illum_position;
  Vec3 hull_min;
  Vec3 hull_max;
  Vec3 view_bbmin;
  Vec3 view_bbmax;
  int flags;
  int num_bones;
  int bone_index;
  int num_bone_controllers;
  int bone_controller_index;
  int num_hitbox_sets;
  int hitbox_set_index;

  studio_hitbox_set* hitbox_set(int index) const {
    if (index < 0 || index >= num_hitbox_sets) {
      return nullptr;
    }

    return ((studio_hitbox_set*)((unsigned char*)this + hitbox_set_index)) + index;
  }
};

class ModelInfo {
public:
  studio_hdr* get_studio_model(const model_t* model) {
    if (model == nullptr) {
      return nullptr;
    }

    void** vtable = *(void***)this;
    auto get_studio_model_fn = (studio_hdr * (*)(void*, const model_t*))vtable[29];
    return get_studio_model_fn(this, model);
  }
};

inline static ModelInfo* model_info;

#endif
