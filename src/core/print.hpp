/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/print.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef PRINT_HPP
#define PRINT_HPP

#include "logger.hpp"

#include <cstdarg>

static void print(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  cathook::core::vlog_raw(fmt, args);
  va_end(args);
}

#endif
