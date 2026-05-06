/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/core/hooks/dispatch_user_message.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "games/tf2/sdk/bitbuf.hpp"
#include "features/automation/misc/misc.hpp"

bool (*dispatch_user_message_original)(void*, int, bf_read*);

bool dispatch_user_message_hook(void* me, int message_type, bf_read* message_data) {
  automation::controller().on_dispatch_user_message(message_type, message_data);
  return dispatch_user_message_original(me, message_type, message_data);
}
