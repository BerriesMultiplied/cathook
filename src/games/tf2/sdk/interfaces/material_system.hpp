/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/interfaces/material_system.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef MATERIAL_SYSTEM_HPP
#define MATERIAL_SYSTEM_HPP

#include "render_context.hpp"

#include "games/tf2/sdk/materials/material.hpp"
#include "games/tf2/sdk/materials/keyvalues.hpp"
#include "games/tf2/sdk/materials/texture.hpp"

enum image_format {
  image_format_unknown = -1,
  image_format_rgba8888 = 0,
  image_format_abgr8888,
  image_format_rgb888,
};

enum material_render_target_depth {
  material_rt_depth_shared = 0,
  material_rt_depth_separate,
  material_rt_depth_none,
  material_rt_depth_only,
};

enum render_target_size_mode {
  rt_size_no_change = 0,
  rt_size_default,
  rt_size_picmip,
  rt_size_hdr,
  rt_size_full_frame_buffer,
  rt_size_offscreen,
  rt_size_full_frame_buffer_rounded_up,
  rt_size_replay_screenshot,
  rt_size_literal,
  rt_size_literal_picmip,
};

constexpr unsigned int texture_flags_clamps = 0x00000004;
constexpr unsigned int texture_flags_clampt = 0x00000008;
constexpr unsigned int texture_flags_eight_bit_alpha = 0x00002000;
constexpr unsigned int create_render_target_flags_hdr = 0x00000001;

class MaterialSystem {
public:
  Material* find_material(char const* material_name, const char* texture_group_name, bool complain, const char* complain_prefix) {
    void** vtable = *(void ***)this;

    Material* (*find_material_fn)(void*, const char*, const char*, bool, const char*) =
      (Material* (*)(void*, const char*, const char*, bool, const char*))vtable[71];

    return find_material_fn(this, material_name, texture_group_name, complain, complain_prefix);
  }

  Material* create_material(const char* name, KeyValues* key_value) {
    void** vtable = *(void ***)this;

    Material* (*create_material_fn)(void*, const char*, void*) = (Material* (*)(void*, const char*, void*))vtable[70];

    return create_material_fn(this, name, key_value);
  }

  Texture* create_named_render_target_texture_ex(
    const char* render_target_name,
    int width,
    int height,
    render_target_size_mode size_mode,
    image_format format,
    material_render_target_depth depth,
    unsigned int texture_flags,
    unsigned int render_target_flags) {
    void** vtable = *(void***)this;

    auto create_named_render_target_texture_ex_fn =
      (Texture* (*)(
        void*,
        const char*,
        int,
        int,
        render_target_size_mode,
        image_format,
        material_render_target_depth,
        unsigned int,
        unsigned int))vtable[85];

    return create_named_render_target_texture_ex_fn(
      this,
      render_target_name,
      width,
      height,
      size_mode,
      format,
      depth,
      texture_flags,
      render_target_flags);
  }

  RenderContext* get_render_context(void) {
    void** vtable = *(void***)this;

    RenderContext* (*get_render_context_fn)(void*) = (RenderContext* (*)(void*))vtable[98];

    return get_render_context_fn(this);
  }

  void set_in_stub_mode(bool enabled) {
    void** vtable = *(void***)this;

    void (*set_in_stub_mode_fn)(void*, bool) = (void (*)(void*, bool))vtable[58];

    set_in_stub_mode_fn(this, enabled);
  }
  
};

inline static MaterialSystem* material_system;

#endif
