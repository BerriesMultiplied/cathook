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
#include <atomic>
#include <chrono>
#include <thread>

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
void** poll_event_target = nullptr;
void** swap_window_target = nullptr;
void** get_window_flags_target = nullptr;
void** get_window_WM_info_target = nullptr;
void** get_window_size_target = nullptr;
SDL_GLContext original_gl_context = NULL;
SDL_GLContext menu_gl_context = NULL;
std::atomic_bool sdl_hooks_installed = false;
std::atomic_bool sdl_hooks_uninstalling = false;
std::atomic_int sdl_active_hook_calls = 0;

struct sdl_hook_call_guard
{
  sdl_hook_call_guard()
  {
    sdl_active_hook_calls.fetch_add(1, std::memory_order_acq_rel);
  }

  ~sdl_hook_call_guard()
  {
    sdl_active_hook_calls.fetch_sub(1, std::memory_order_acq_rel);
  }
};

void begin_sdl_hook_uninstall()
{
  sdl_hooks_uninstalling.store(true, std::memory_order_release);

  for (int attempt = 0; attempt < 100 && sdl_active_hook_calls.load(std::memory_order_acquire) > 0; ++attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void finish_sdl_hook_uninstall()
{
  sdl_hooks_installed.store(false, std::memory_order_release);
  sdl_hooks_uninstalling.store(false, std::memory_order_release);
}

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
  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    return 1;
  }

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
  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    return poll_event_original != nullptr ? poll_event_original(event) : false;
  }

  sdl_hook_call_guard guard{};

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
  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    return peep_events_original != nullptr ? peep_events_original(events, numevents, action, min, max) : -1;
  }

  sdl_hook_call_guard guard{};

  int ret = peep_events_original(events, numevents, action, min, max);

  /*
  if(ret != -1 && sdl_window != nullptr && ImGui::GetCurrentContext())
    ImGui_ImplSDL2_ProcessEvent(events);

  get_input(events);
  */
  
  return ret;
}


void swap_window_hook(SDL_Window* window) {
  void (*original)(void*) = swap_window_original;

  if (original == nullptr) {
    return;
  }

  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    original(window);
    return;
  }

  {
    sdl_hook_call_guard guard{};

    if (nographics::should_skip_rendering_hooks()) {
      original(window);
    } else {
      if (!menu_gl_context) {
        original_gl_context = SDL_GL_GetCurrentContext();
        menu_gl_context = SDL_GL_CreateContext(window);
        if (menu_gl_context == nullptr) {
          print("Failed to create menu GL context\n");
          original(window);
          return;
        }

        GLenum err = glewInit();
        if (err != GLEW_OK) {
          print("Failed to initialize GLEW: %s\n", glewGetErrorString(err));
          original(window);
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
      draw_backtrack_visualizer_imgui();
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

      original(window);
    }
  }

  cathook::core::service_detach_request();
}

Uint32 get_window_flags_hook(SDL_Window* window) {
  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    return get_window_flags_original != nullptr ? get_window_flags_original(window) : 0;
  }

  sdl_hook_call_guard guard{};

  if (get_window_flags_original == nullptr) {
    return 0;
  }

  return get_window_flags_original(window);
}

SDL_bool get_window_WM_info_hook(SDL_Window* window, SDL_SysWMinfo* info) {
  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    return get_window_WM_info_original != nullptr ? get_window_WM_info_original(window, info) : SDL_FALSE;
  }

  sdl_hook_call_guard guard{};

  if (get_window_WM_info_original == nullptr) {
    return SDL_FALSE;
  }

  return get_window_WM_info_original(window, info);
}

// Used for grabbing the SDL_Window handle when in Vulkan mode
void get_window_size_hook(SDL_Window* window, int* w, int* h) {
  if (sdl_hooks_uninstalling.load(std::memory_order_acquire)) {
    if (get_window_size_original != nullptr) {
      get_window_size_original(window, w, h);
      return;
    }
  }

  sdl_hook_call_guard guard{};

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
