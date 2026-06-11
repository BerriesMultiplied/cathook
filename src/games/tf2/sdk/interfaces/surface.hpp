/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/games/tf2/sdk/interfaces/surface.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef SURFACE_HPP
#define SURFACE_HPP

#include <atomic>
#include <wchar.h>

#include "core/types.hpp"

#include "features/menu/config.hpp"

namespace surface_runtime
{

inline std::atomic_bool ready = false;

inline bool is_ready()
{
  return ready.load(std::memory_order_acquire);
}

inline void mark_ready()
{
  ready.store(true, std::memory_order_release);
}

inline void reset_ready()
{
  ready.store(false, std::memory_order_release);
}

} // namespace surface_runtime

class Surface {
public:
  void set_rgba(int r, int g, int b, int a) {
    void** vtable = *(void ***)this;

    void (*set_rgba_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[10];

    set_rgba_fn(this, r, g, b, a);
  }

  void set_rgba(RGBA color) {
    void** vtable = *(void ***)this;

    void (*set_rgba_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[10];

    set_rgba_fn(this, color.r, color.g, color.b, color.a);
  }

  
  void draw_line(int x1, int y1, int x2, int y2) {
    void** vtable = *(void ***)this;

    void (*draw_line_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[15];

    draw_line_fn(this, x1, y1, x2, y2);
  }

  
  void draw_filled_rect(int x1, int y1, int x2, int y2) {
    void** vtable = *(void ***)this;

    void (*draw_filled_rect_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[12];

    draw_filled_rect_fn(this, x1, y1, x2, y2);
  }

  void draw_outlined_rect(int x1, int y1, int x2, int y2) {
    void** vtable = *(void ***)this;

    void (*draw_outlined_rect_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[14];

    draw_outlined_rect_fn(this, x1, y1, x2, y2);
  }

  Vec2 get_screen_size(void) {
    void** vtable = *(void ***)this;

    void (*get_screen_size_fn)(void*, int*, int*) = (void (*)(void*, int*, int*))vtable[44];

    int width = 0;
    int height = 0;
    get_screen_size_fn(this, &width, &height);

    return Vec2{width, height};
  }
  
  unsigned long text_create_font(void) {
    void** vtable = *(void ***)this;

    unsigned long (*text_create_font_fn)(void*) = (unsigned long (*)(void*))vtable[66];

    return text_create_font_fn(this);
  }

  char text_set_font_glyph_set(unsigned long font, const char *name, int tall, int weight, int blur, int scanlines, int flags) {
    void** vtable = *(void ***)this;

    char (*text_set_font_glyph_set_fn)(void*, unsigned long, const char*, int, int, int, int, int, int, int) =
      (char (*)(void*, unsigned long, const char*, int, int, int, int, int, int, int))vtable[67];

    return text_set_font_glyph_set_fn(this, font, name, tall, weight, blur, scanlines, flags, 0, 0);
  }

  int get_font_height(unsigned long font) {
    return config.debug.font_height; //CHANGE ME: use virtual function GetFontTall instead
  }
  
  int get_character_width(unsigned long font, int character) {
    void** vtable = *(void ***)this;

    int (*draw_set_text_font_fn)(void*, unsigned long, int) = (int (*)(void*, unsigned long, int))vtable[74];

    return draw_set_text_font_fn(this, font, character);
  }

  unsigned int get_string_width(unsigned long font, const wchar_t* string) {
    unsigned int string_width = 0;
    for (unsigned int i = 0; i < wcslen(string); ++i) {
      string_width += get_character_width(font, string[i]);
    }

    return string_width;
  }
  
  void draw_set_text_font(unsigned long font) {
    void** vtable = *(void ***)this;

    void (*draw_set_text_font_fn)(void*, unsigned long) = (void (*)(void*, unsigned long))vtable[17];

    draw_set_text_font_fn(this, font);
  }

  void draw_set_text_color(int r, int g, int b, int a) {
    void** vtable = *(void ***)this;

    void (*draw_set_text_color_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[18];

    draw_set_text_color_fn(this, r, g, b, a);
  }

  void draw_set_text_color(RGBA color) {
    void** vtable = *(void ***)this;

    void (*draw_set_text_color_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[18];

    draw_set_text_color_fn(this, color.r, color.g, color.b, color.a);
  }
  
  void draw_set_text_pos(unsigned int x, unsigned int y) {
    void** vtable = *(void ***)this;

    void (*draw_set_text_pos_fn)(void*, unsigned int, unsigned int) = (void (*)(void*, unsigned int, unsigned int))vtable[20];

    draw_set_text_pos_fn(this, x, y);
  }

  void draw_print_text(const wchar_t *text, int text_len) {
    void** vtable = *(void ***)this;

    void (*draw_print_text_fn)(void*, const wchar_t*, int, int) = (void (*)(void*, const wchar_t*, int, int))vtable[22];

    draw_print_text_fn(this, text, text_len, 0);
  }

  void set_cursor_visible(bool visible) {
    void** vtable = *(void ***)this;

    void (*set_cursor_visible_fn)(void*, bool) = (void (*)(void*, bool))vtable[52];

    set_cursor_visible_fn(this, visible);
  }

  void draw_circle(int x, int y, int radius, int segments) {
    void** vtable = *(void ***)this;

    void (*draw_circle_fn)(void*, int, int, int, int) = (void (*)(void*, int, int, int, int))vtable[99];

    draw_circle_fn(this, x, y, radius, segments);
  }

};

static inline Surface* surface;

#endif
