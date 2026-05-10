/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/sdl.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_syswm.h>
#include <GL/glew.h>

#include "imgui/dearimgui.hpp"

#include "games/tf2/sdk/interfaces/surface.hpp"
#include "games/tf2/sdk/interfaces/engine.hpp"

#include "core/print.hpp"

#include "features/menu/menu.hpp"
#include "features/menu/indicators.hpp"
#include "features/combat/backtrack/backtrack.hpp"
#include "features/visuals/esp/esp.hpp"
#include "features/visuals/hitmarker.hpp"
#include "features/visuals/spectator_list.hpp"
#include "features/automation/navbot/navbot_controller.hpp"
#include "features/automation/nographics/nographics.hpp"

bool (*poll_event_original)(SDL_Event*) = NULL;
int  (*peep_events_original)(SDL_Event*, int, SDL_eventaction, int, int) = NULL;
void (*swap_window_original)(void*) = NULL;
Uint32 (*get_window_flags_original)(SDL_Window*) = NULL;
SDL_bool (*get_window_WM_info_original)(SDL_Window* window, SDL_SysWMinfo* info) = NULL;
void (*get_window_size_original)(SDL_Window* window, int* w, int* h) = NULL;
SDL_GLContext original_gl_context = NULL;
SDL_GLContext menu_gl_context = NULL;

static void update_imgui_sdl_display_size(SDL_Window* window) {
  if (window == nullptr) {
    return;
  }

  int window_width = 0;
  int window_height = 0;
  int drawable_width = 0;
  int drawable_height = 0;
  SDL_GetWindowSize(window, &window_width, &window_height);
  SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);

  auto display_width = window_width;
  auto display_height = window_height;
  if (engine != nullptr) {
    const auto engine_size = engine->get_screen_size();
    if (engine_size.x > 0 && engine_size.y > 0) {
      display_width = engine_size.x;
      display_height = engine_size.y;
    }
  }

  if (display_width <= 0 || display_height <= 0) {
    return;
  }

  auto& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(display_width), static_cast<float>(display_height));
  io.DisplayFramebufferScale = ImVec2(
    drawable_width > 0 ? static_cast<float>(drawable_width) / static_cast<float>(display_width) : 1.0f,
    drawable_height > 0 ? static_cast<float>(drawable_height) / static_cast<float>(display_height) : 1.0f);
}

// This filters key events to the game
int SDLCALL event_filter(void* userdata, SDL_Event* event) {
  if (event == nullptr) {
    return 1;
  }

  if (menu_focused == false) return 1; // Don't filter anything if the menu is closed

  const bool imgui_ready = sdl_window != nullptr && ImGui::IsImGuiFullyInitialized();
  if (imgui_ready) {
    ImGui_ImplSDL2_ProcessEvent(event);
  }

  get_input(event);
  if (!imgui_ready) {
    return 1;
  }

  // Allow keys to be released
  if (event->type == SDL_KEYUP) {
    return 1;
  }
  
  // Block inputs to the game when editing input boxes
  if (imgui_ready && ImGui::IsAnyItemActive() == true && ImGui::IsMouseDown(ImGuiMouseButton_Left) != true) return 0;
  
  // Only some movement keys
  if (event->type == SDL_KEYDOWN) {
    SDL_KeyboardEvent* key = &event->key;
    SDL_Keycode sym = key->keysym.sym;
    if (sym == SDLK_w || sym == SDLK_a || sym == SDLK_s || sym == SDLK_d || sym == SDLK_INSERT ||
	sym == SDLK_SPACE || sym == SDLK_LCTRL) {
      return 1;
    }
  }
  
  // Block everything else
  return 0;
}


bool poll_event_hook(SDL_Event* event) {
  if (poll_event_original == nullptr) {
    return false;
  }

  const bool ret = poll_event_original(event);
  if (!ret || event == nullptr) {
    return ret;
  }
  
  if (sdl_window != nullptr && ImGui::IsImGuiFullyInitialized()) {
    ImGui_ImplSDL2_ProcessEvent(event);
  }
  
  get_input(event);
  
  return ret;
}


int peep_events_hook(SDL_Event* events, int numevents, SDL_eventaction action, int min, int max) {
  int ret = peep_events_original(events, numevents, action, min, max);

  /*
  if(ret != -1 && sdl_window != nullptr && ImGui::GetCurrentContext())
    ImGui_ImplSDL2_ProcessEvent(events);

  get_input(events);
  */
  
  return ret;
}


void swap_window_hook(SDL_Window* window) {
  if (swap_window_original == nullptr) {
    return;
  }

  if (nographics::should_skip_rendering_hooks()) {
    swap_window_original(window);
    cathook::core::service_detach_request();
    return;
  }

  if (!menu_gl_context) {
    original_gl_context = SDL_GL_GetCurrentContext();
    menu_gl_context = SDL_GL_CreateContext(window);
    if (menu_gl_context == nullptr) {
      print("Failed to create menu GL context\n");
      swap_window_original(window);
      return;
    }

    GLenum err = glewInit();
    if (err != GLEW_OK) {
      print("Failed to initialize GLEW: %s\n", glewGetErrorString(err));
      swap_window_original(window);
      return;
    }
    
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 100");
    ImGui_ImplSDL2_InitForOpenGL(window, nullptr);    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    orig_style = ImGui::GetStyle();

    set_imgui_theme();
  }

  
  SDL_GL_MakeCurrent(window, menu_gl_context);
  
  if (ImGui::IsKeyPressed(ImGuiKey_Insert, false) || ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
    menu_focused = !menu_focused;
    if (surface != nullptr) {
      surface->set_cursor_visible(menu_focused);
    }
  }
  
  cat_menu::ensure_fonts();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  update_imgui_sdl_display_size(window);
  ImGui::NewFrame();

  draw_aimbot_fov_imgui();
  draw_thirdperson_crosshair_imgui();
  draw_players_imgui();
  backtrack::draw_visualizer_imgui();
  draw_projectile_debug_imgui();
  hitmarker::draw_imgui();
  navbot::controller().draw_imgui();

  draw_watermark();

  draw_game_indicators();

  if (menu_focused == true) {
    draw_menu();
  }  

  
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_GL_MakeCurrent(window, original_gl_context);

  swap_window_original(window);
  cathook::core::service_detach_request();
}

Uint32 get_window_flags_hook(SDL_Window* window) {
  if (get_window_flags_original == nullptr) {
    return 0;
  }

  return get_window_flags_original(window);
}

SDL_bool get_window_WM_info_hook(SDL_Window* window, SDL_SysWMinfo* info) {
  if (get_window_WM_info_original == nullptr) {
    return SDL_FALSE;
  }

  return get_window_WM_info_original(window, info);
}

// Used for grabbing the SDL_Window handle when in Vulkan mode
void get_window_size_hook(SDL_Window* window, int* w, int* h) {
  if (get_window_size_original == nullptr) {
    if (w != nullptr) {
      *w = 0;
    }
    if (h != nullptr) {
      *h = 0;
    }
    return;
  }

  sdl_window = window;
  
  get_window_size_original(window, w, h);
}
