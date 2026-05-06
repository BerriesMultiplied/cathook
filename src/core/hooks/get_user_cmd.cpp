/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/get_user_cmd.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "games/tf2/sdk/interfaces/input.hpp"

user_cmd* get_user_cmd(void* me, int sequence_number) {
  return input->get_user_cmd(sequence_number);
}
