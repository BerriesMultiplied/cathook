/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/assert.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include <SDL2/SDL.h>

#include "core/logger.hpp"

// Create an error window with a custom message
#define error_box(message) \
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,			\
			   "Fatal Error",				\
			   message,					\
			   nullptr);					

// Asserts if expression is true
// Creates an error window with a custom message, and then exits the program.
#define error_assert(expression, message)				\
  if (expression) {							\
    cathook::core::log_raw("fatal error: ");				\
    cathook::core::log_raw(message);					\
    error_box(message);							\
    exit(1);								\
  }
