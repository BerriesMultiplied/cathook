/*
/^-----^\   data: 2026-03-30
V  o o  V  file: src/core/hooks/client_create_move.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "games/tf2/sdk/interfaces/global_vars.hpp"
#include "games/tf2/sdk/interfaces/input.hpp"
#include "games/tf2/sdk/interfaces/steam_friends.hpp"

#include "games/tf2/sdk/entities/player.hpp"

#include "core/entity_cache.hpp"
#include "core/detach.hpp"
#include "features/combat/anti_aim/anti_aim.hpp"
#include "features/combat/random_crits/random_crits.hpp"
#include "features/combat/tickbase/tickbase.hpp"

void (*client_create_move_original)(void*, int, float, bool);

namespace
{

unsigned int crc32_process_byte(unsigned int crc, unsigned char value)
{
  crc ^= value;
  for (int bit = 0; bit < 8; ++bit) {
    const unsigned int mask = 0U - (crc & 1U);
    crc = (crc >> 1) ^ (0xEDB88320U & mask);
  }

  return crc;
}

unsigned int crc32_process_buffer(unsigned int crc, const void* data, int size)
{
  const auto* bytes = static_cast<const unsigned char*>(data);
  for (int i = 0; i < size; ++i) {
    crc = crc32_process_byte(crc, bytes[i]);
  }

  return crc;
}

unsigned int user_cmd_checksum(const user_cmd& cmd)
{
  unsigned int crc = 0xFFFFFFFFU;
  crc = crc32_process_buffer(crc, &cmd.command_number, sizeof(cmd.command_number));
  crc = crc32_process_buffer(crc, &cmd.tick_count, sizeof(cmd.tick_count));
  crc = crc32_process_buffer(crc, &cmd.view_angles, sizeof(cmd.view_angles));
  crc = crc32_process_buffer(crc, &cmd.forwardmove, sizeof(cmd.forwardmove));
  crc = crc32_process_buffer(crc, &cmd.sidemove, sizeof(cmd.sidemove));
  crc = crc32_process_buffer(crc, &cmd.upmove, sizeof(cmd.upmove));
  crc = crc32_process_buffer(crc, &cmd.buttons, sizeof(cmd.buttons));
  crc = crc32_process_buffer(crc, &cmd.impulse, sizeof(cmd.impulse));
  crc = crc32_process_buffer(crc, &cmd.weapon_select, sizeof(cmd.weapon_select));
  crc = crc32_process_buffer(crc, &cmd.weapon_subtype, sizeof(cmd.weapon_subtype));
  crc = crc32_process_buffer(crc, &cmd.random_seed, sizeof(cmd.random_seed));
  crc = crc32_process_buffer(crc, &cmd.mouse_dx, sizeof(cmd.mouse_dx));
  crc = crc32_process_buffer(crc, &cmd.mouse_dy, sizeof(cmd.mouse_dy));
  return crc ^ 0xFFFFFFFFU;
}

void update_verified_user_cmd(int sequence_number, user_cmd* cmd)
{
  if (input == nullptr || cmd == nullptr) {
    return;
  }

  auto* verified_cmd = input->get_verified_user_cmd(sequence_number);
  if (verified_cmd == nullptr) {
    return;
  }

  verified_cmd->cmd = *cmd;
  verified_cmd->crc = user_cmd_checksum(*cmd);
}

} // namespace



void client_create_move_hook(void* me, int sequence_number, float input_sample_frametime, bool active) {
  client_create_move_original(me, sequence_number, input_sample_frametime, active);

  if (cathook::core::is_detach_pending()) {
    cathook::core::service_detach_request();
    return;
  }

  if (input == nullptr) {
    return;
  }

  auto* user_cmd = input->get_user_cmd(sequence_number);
  if (user_cmd == nullptr) {
    return;
  }

  random_crits::run(user_cmd);
  tickbase::on_create_move(user_cmd);
  anti_aim::on_create_move(user_cmd);
  update_verified_user_cmd(sequence_number, user_cmd);
  
  /*
  user_cmd* user_cmd = input->get_user_cmd(sequence_number);
  if (user_cmd == nullptr) {
    print("user_cmd == nullptr\n");
    return;
  }
  */
  
  /*
  prediction->update(client_state->m_nDeltaTick, client_state->m_nDeltaTick > 0, client_state->last_command_ack,
		     client_state->lastoutgoingcommand + client_state->chokedcommands);
  */
  
  //print("%p - %d - %d - %d - %f - %d\n", user_cmd, user_cmd->tick_count, sequence_number, sequence_number%90, input_sample_frametime, active);

  //user_cmd->random_seed = MD5_PseudoRandom(user_cmd->command_number) & 0x7FFFFFFF;

  //bhop(user_cmd);

  //print("%d\n", user_cmd->buttons);
  
  //print("client create move hooked!\n");
}
