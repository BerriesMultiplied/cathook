/*
/^-----^\   data: 2026-05-01
V  o o  V  file: src/games/tf2/sdk/interfaces/net_channel.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef NET_CHANNEL_HPP
#define NET_CHANNEL_HPP

#include <cstdint>

#include "games/tf2/sdk/bitbuf.hpp"

class net_message;
class net_channel_handler;

class net_channel_info {
public:
  virtual auto get_name() const -> const char* = 0;
  virtual auto get_address() const -> const char* = 0;
  virtual auto get_time() const -> float = 0;
  virtual auto get_time_connected() const -> float = 0;
  virtual auto get_buffer_size() const -> int = 0;
  virtual auto get_data_rate() const -> int = 0;
  virtual auto is_loopback() const -> bool = 0;
  virtual auto is_timing_out() const -> bool = 0;
  virtual auto is_playback() const -> bool = 0;
  virtual auto get_latency(int flow) const -> float = 0;
  virtual auto get_avg_latency(int flow) const -> float = 0;
  virtual auto get_avg_loss(int flow) const -> float = 0;
  virtual auto get_avg_choke(int flow) const -> float = 0;
  virtual auto get_avg_data(int flow) const -> float = 0;
  virtual auto get_avg_packets(int flow) const -> float = 0;
  virtual auto get_total_data(int flow) const -> int = 0;
  virtual auto get_sequence_number(int flow) const -> int = 0;
  virtual auto is_valid_packet(int flow, int frame_number) const -> bool = 0;
  virtual auto get_packet_time(int flow, int frame_number) const -> float = 0;
  virtual auto get_packet_bytes(int flow, int frame_number, int group) const -> int = 0;
  virtual auto get_stream_progress(int flow, int* received, int* total) const -> bool = 0;
  virtual auto get_time_since_last_received() const -> float = 0;
  virtual auto get_command_interpolation_amount(int flow, int frame_number) const -> float = 0;
  virtual void get_packet_response_latency(int flow, int frame_number, int* latency_msecs, int* choke) const = 0;
  virtual void get_remote_framerate(float* frame_time, float* frame_time_std_deviation) const = 0;
  virtual auto get_timeout_seconds() const -> float = 0;

  enum group {
    generic = 0,
    localplayer,
    otherplayers,
    entities,
    sounds,
    events,
    usermessages,
    entmessages,
    voice,
    stringtable,
    move,
    stringcmd,
    signon,
    total,
  };
};

class net_channel : public net_channel_info {
public:
  virtual ~net_channel() = default;
  virtual void set_data_rate(float rate) = 0;
  virtual auto register_message(net_message* message) -> bool = 0;
  virtual void set_timeout(float seconds) = 0;
  virtual void set_demo_recorder(void* recorder) = 0;
  virtual void set_challenge_number(unsigned int challenge_number) = 0;
  virtual void reset() = 0;
  virtual void clear() = 0;
  virtual void shutdown(const char* reason) = 0;
  virtual void process_playback() = 0;
  virtual void process_packet(void* packet, bool has_header) = 0;
  virtual auto send_net_msg(net_message& message, bool force_reliable = false, bool voice = false) -> bool = 0;
  virtual auto send_data(bf_write& message, bool reliable = true) -> bool = 0;
  virtual auto send_file(const char* filename, unsigned int transfer_id) -> bool = 0;
  virtual void deny_file(const char* filename, unsigned int transfer_id) = 0;
  virtual void request_file_old(const char* filename, unsigned int transfer_id) = 0;
  virtual void set_choked() = 0;
  virtual auto send_datagram(bf_write* data) -> int = 0;
  virtual auto transmit(bool only_reliable = false) -> bool = 0;
  virtual auto get_remote_address() const -> const void* = 0;
  virtual auto get_msg_handler() const -> net_channel_handler* = 0;
  virtual auto get_drop_number() const -> int = 0;
  virtual auto get_socket() const -> int = 0;
  virtual auto get_challenge_number() const -> unsigned int = 0;
  virtual void get_sequence_data(int& out_sequence, int& in_sequence, int& out_sequence_ack) = 0;
  virtual void set_sequence_data(int out_sequence, int in_sequence, int out_sequence_ack) = 0;
  virtual void update_message_stats(int message_group, int bits) = 0;
  virtual auto can_packet() const -> bool = 0;
  virtual auto is_overflowed() const -> bool = 0;
  virtual auto is_timed_out() const -> bool = 0;
  virtual auto has_pending_reliable_data() -> bool = 0;
  virtual void set_file_transmission_mode(bool background_mode) = 0;
  virtual void set_compression_mode(bool use_compression) = 0;
  virtual auto request_file(const char* filename) -> unsigned int = 0;
  virtual auto get_time_since_last_received_net_channel() const -> float = 0;
  virtual void set_max_buffer_size(bool reliable, int bytes, bool voice = false) = 0;
  virtual auto is_null() const -> bool = 0;
  virtual auto get_num_bits_written(bool reliable) -> int = 0;
  virtual void set_interpolation_amount(float interpolation_amount) = 0;
  virtual void set_remote_framerate(float frame_time, float frame_time_std_deviation) = 0;
  virtual void set_max_routable_payload_size(int split_size) = 0;
  virtual auto get_max_routable_payload_size() -> int = 0;
  virtual auto get_protocol_version() -> int = 0;
};

class net_channel_storage : public net_channel {
public:
  enum class connection_status : int {
    disconnected = 0,
    connecting,
    connection_failed,
    connected,
  };

  connection_status connection_state;
  int out_sequence_number;
  int in_sequence_number;
  int out_sequence_number_ack;
  int out_reliable_state;
  int in_reliable_state;
  int choked_packets;
};

#endif
