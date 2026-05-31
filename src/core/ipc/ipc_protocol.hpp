/*
/^-----^\   data: 2026-05-05
V  o o  V  file: src/core/ipc/ipc_protocol.hpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef CAT_IPC_PROTOCOL_HPP
#define CAT_IPC_PROTOCOL_HPP

#include <cstdint>
#include <ctime>
#include <pthread.h>
#include <sys/types.h>

namespace cat_ipc
{

constexpr const char* ipc_socket_path = "/opt/cathook/ipc/cathook_followbot_server";
constexpr std::uint32_t cathook_magic_number = 0x0deadca7u;
constexpr std::uint32_t max_peers = 255;
constexpr std::uint32_t command_ring_size = max_peers * 2;
constexpr std::uint32_t command_data_size = 64;
constexpr std::uint32_t command_payload_size = 4096;
constexpr std::uint32_t command_pool_size = command_ring_size * command_payload_size;
constexpr std::uint32_t peer_dead_seconds = 10;

namespace commands
{
constexpr std::uint32_t execute_client_cmd = 1;
constexpr std::uint32_t set_follow_steamid = 2;
constexpr std::uint32_t execute_client_cmd_long = 3;
constexpr std::uint32_t move_to_vector = 4;
constexpr std::uint32_t stop_moving = 5;
constexpr std::uint32_t start_moving = 6;
} // namespace commands

struct server_data_s
{
  unsigned int magic_number;
};

struct user_data_s
{
  char name[32];
  unsigned int friendid;
  bool textmode;
  bool connected;
  std::time_t heartbeat;
  std::time_t ts_injected;
  std::time_t ts_connected;
  std::time_t ts_disconnected;
  std::time_t ts_queue_started;

  struct accumulated_t
  {
    int kills;
    int deaths;
    int score;
    int shots;
    int hits;
    int headshots;
  } accumulated;

  struct ingame_t
  {
    bool good;
    int kills;
    int deaths;
    int score;
    int shots;
    int hits;
    int headshots;
    int team;
    int role;
    char life_state;
    int health;
    int health_max;
    float x;
    float y;
    float z;
    int player_count;
    int bot_count;
    char server[24];
    char mapname[32];
  } ingame;
};

struct peer_data_s
{
  bool free;
  std::time_t heartbeat;
  pid_t pid;
  unsigned long starttime;
};

struct command_s
{
  unsigned int command_number;
  int target_peer;
  int sender;
  unsigned long payload_offset;
  unsigned int payload_size;
  unsigned int cmd_type;
  unsigned char cmd_data[command_data_size];
};

struct shared_state
{
  pthread_mutex_t mutex;
  unsigned int peer_count;
  unsigned long command_count;
  peer_data_s peer_data[max_peers];
  command_s commands[command_ring_size];
  unsigned char pool[command_pool_size];
  server_data_s global_data;
  user_data_s peer_user_data[max_peers];
};

} // namespace cat_ipc

#endif
