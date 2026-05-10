/*
/^-----^\   data: 2026-05-10
V  o o  V  file: src/features/combat/backtrack/backtrack.cpp
 |  Y  |   author: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/combat/backtrack/backtrack.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <optional>

#include "imgui/dearimgui.hpp"

#include "features/combat/aimbot/aim_utils.hpp"
#include "features/menu/config.hpp"
#include "features/movement/local_prediction/local_prediction.hpp"
#include "features/visuals/overlay_projection.hpp"

#include "games/tf2/sdk/interfaces/client_state.hpp"
#include "games/tf2/sdk/interfaces/convar_system.hpp"
#include "games/tf2/sdk/interfaces/engine.hpp"
#include "games/tf2/sdk/interfaces/engine_trace.hpp"
#include "games/tf2/sdk/interfaces/entity_list.hpp"
#include "games/tf2/sdk/interfaces/global_vars.hpp"
#include "games/tf2/sdk/interfaces/model_info.hpp"

bool write_to_table(void** vtable, int index, void* func);

namespace backtrack
{

namespace
{

constexpr int flow_outgoing = 0;
constexpr int net_channel_send_datagram_index = 44;
constexpr float max_unlag_seconds = 1.0f;

struct incoming_sequence {
  int reliable_state = 0;
  int sequence_number = 0;
  float realtime = 0.0f;
};

using send_datagram_fn = int (*)(net_channel*, bf_write*);

std::array<player_records, max_entities> g_records{};
std::deque<incoming_sequence> g_sequences{};
int g_last_incoming_sequence = 0;
float g_latency_ramp = 0.0f;
std::optional<Vec3> g_selected_position{};
void** g_hooked_net_channel_vtable = nullptr;
send_datagram_fn g_send_datagram_original = nullptr;

constexpr std::array<int, max_points> tracked_hitbox_ids = {
  aim_hitbox_head,
  aim_hitbox_spine_3,
  aim_hitbox_spine_2,
  aim_hitbox_spine_1,
  aim_hitbox_spine_0,
  aim_hitbox_pelvis,
  aim_hitbox_left_upper_arm,
  aim_hitbox_right_upper_arm,
  aim_hitbox_left_thigh,
  aim_hitbox_right_thigh
};

[[nodiscard]] float window_seconds()
{
  return std::clamp(static_cast<float>(config.backtrack.window_ms) * 0.001f, 0.0f, max_unlag_seconds);
}

[[nodiscard]] net_channel* current_net_channel()
{
  return client_state != nullptr ? client_state->m_NetChannel : nullptr;
}

[[nodiscard]] bool localplayer_valid()
{
  if (entity_list == nullptr) {
    return false;
  }

  auto* localplayer = entity_list->get_localplayer();
  return localplayer != nullptr && localplayer->is_alive();
}

[[nodiscard]] bool should_run_network_state()
{
  auto* channel = current_net_channel();
  return is_enabled() &&
         engine != nullptr &&
         global_vars != nullptr &&
         engine->is_in_game() &&
         channel != nullptr &&
         !channel->is_loopback() &&
         localplayer_valid();
}

[[nodiscard]] bool should_record()
{
  return is_enabled() &&
         engine != nullptr &&
         global_vars != nullptr &&
         engine->is_in_game();
}

[[nodiscard]] float teleport_distance_sqr()
{
  static Convar* sv_lagcompensation_teleport_dist = nullptr;
  if (sv_lagcompensation_teleport_dist == nullptr && convar_system != nullptr) {
    sv_lagcompensation_teleport_dist = convar_system->find_var("sv_lagcompensation_teleport_dist");
  }

  const float distance = sv_lagcompensation_teleport_dist != nullptr
    ? std::max(sv_lagcompensation_teleport_dist->get_float(), 1.0f)
    : 64.0f;
  return distance * distance;
}

[[nodiscard]] backtrack_bounds get_player_bounds(Player* target, float expansion = 0.0f)
{
  backtrack_bounds bounds{};
  if (target == nullptr) {
    return bounds;
  }

  const Vec3 origin = target->get_collision_origin();
  const Vec3 mins = target->get_collideable_mins() + origin - Vec3{ expansion, expansion, expansion };
  const Vec3 maxs = target->get_collideable_maxs() + origin + Vec3{ expansion, expansion, expansion };
  if (!aimbot_vec3_is_finite(mins) || !aimbot_vec3_is_finite(maxs)) {
    return bounds;
  }

  if (maxs.x <= mins.x + 1.0f || maxs.y <= mins.y + 1.0f || maxs.z <= mins.z + 8.0f) {
    return bounds;
  }

  bounds.valid = true;
  bounds.mins = mins;
  bounds.maxs = maxs;
  return bounds;
}

[[nodiscard]] std::uint32_t configured_hitbox_mask()
{
  const std::uint32_t mask = config.aimbot.hitscan_hitboxes & aim_hitbox_mask_all;
  return mask != 0 ? mask : aim_hitbox_mask_default_hitscan;
}

[[nodiscard]] Vec3 command_angles(Player* localplayer, const Vec3& bullet_angles)
{
  return localplayer != nullptr ? bullet_angles - localplayer->get_punch_angles() : bullet_angles;
}

[[nodiscard]] bool world_clear(const Vec3& start_pos, const Vec3& end_pos)
{
  if (engine_trace == nullptr || !aimbot_vec3_is_finite(start_pos) || !aimbot_vec3_is_finite(end_pos)) {
    return false;
  }

  Vec3 start = start_pos;
  Vec3 end = end_pos;
  ray_t ray = engine_trace->init_ray(&start, &end);
  trace_filter filter{};
  engine_trace->init_world_trace_filter(&filter);

  trace_t trace_world{};
  engine_trace->trace_ray(&ray, aimbot_hitscan_trace_mask(), &filter, &trace_world);
  return !trace_world.all_solid && !trace_world.start_solid && trace_world.fraction >= 0.999f;
}

[[nodiscard]] bool ray_hits_record(const backtrack_record& record,
  int hitbox_id,
  const Vec3& start_pos,
  const Vec3& end_pos)
{
  if (!record.bounds.valid) {
    return false;
  }

  if (hitbox_id < 0) {
    return aimbot_segment_intersects_aabb(start_pos, end_pos, record.bounds.mins, record.bounds.maxs);
  }

  const Vec3 center = (record.bounds.mins + record.bounds.maxs) * 0.5f;
  const float height = std::max(record.bounds.maxs.z - record.bounds.mins.z, 1.0f);
  float z_min_ratio = 0.08f;
  float z_max_ratio = 0.92f;
  float xy_scale = 0.65f;

  switch (hitbox_id) {
  case aim_hitbox_head:
    z_min_ratio = 0.76f;
    z_max_ratio = 1.0f;
    xy_scale = 0.28f;
    break;
  case aim_hitbox_pelvis:
    z_min_ratio = 0.28f;
    z_max_ratio = 0.52f;
    xy_scale = 0.48f;
    break;
  case aim_hitbox_spine_0:
    z_min_ratio = 0.36f;
    z_max_ratio = 0.60f;
    xy_scale = 0.48f;
    break;
  case aim_hitbox_spine_1:
  case aim_hitbox_spine_2:
  case aim_hitbox_spine_3:
    z_min_ratio = 0.46f;
    z_max_ratio = 0.78f;
    xy_scale = 0.46f;
    break;
  case aim_hitbox_left_thigh:
  case aim_hitbox_right_thigh:
    z_min_ratio = 0.02f;
    z_max_ratio = 0.44f;
    xy_scale = 0.42f;
    break;
  default:
    break;
  }

  const float half_x = std::max((record.bounds.maxs.x - record.bounds.mins.x) * 0.5f * xy_scale, 3.0f);
  const float half_y = std::max((record.bounds.maxs.y - record.bounds.mins.y) * 0.5f * xy_scale, 3.0f);
  const Vec3 mins{
    center.x - half_x,
    center.y - half_y,
    record.bounds.mins.z + (height * z_min_ratio)
  };
  const Vec3 maxs{
    center.x + half_x,
    center.y + half_y,
    record.bounds.mins.z + (height * z_max_ratio)
  };
  return aimbot_segment_intersects_aabb(start_pos, end_pos, mins, maxs);
}

[[nodiscard]] bool point_visible(Player* localplayer,
  const backtrack_record& record,
  const backtrack_point& point)
{
  if (localplayer == nullptr || !point.valid || !aimbot_vec3_is_finite(point.position)) {
    return false;
  }

  const Vec3 start_pos = localplayer->get_shoot_pos();
  if (!aimbot_vec3_is_finite(start_pos) || !world_clear(start_pos, point.position)) {
    return false;
  }

  const Vec3 bullet_angles = aimbot_calculate_angles_to_position(start_pos, point.position);
  Vec3 forward{};
  angle_vectors(bullet_angles, &forward, nullptr, nullptr);
  if (!aimbot_vec3_is_finite(forward)) {
    return false;
  }

  const float target_distance = distance_3d(start_pos, point.position);
  const Vec3 end_pos = start_pos + (forward * std::max(target_distance + 64.0f, 128.0f));
  return ray_hits_record(record, point.hitbox, start_pos, end_pos);
}

[[nodiscard]] bool record_valid(const backtrack_record& record, Player* player)
{
  if (!record.valid ||
      record.player != player ||
      record.ent_index <= 0 ||
      record.point_count <= 0 ||
      !record.bounds.valid ||
      global_vars == nullptr) {
    return false;
  }

  const float age = global_vars->curtime - record.curtime;
  return std::isfinite(age) && age >= 0.0f && age <= window_seconds();
}

[[nodiscard]] bool build_record(Player* player, backtrack_record* record)
{
  if (player == nullptr || record == nullptr || global_vars == nullptr || player->is_dormant() || !player->is_alive()) {
    return false;
  }

  const int ent_index = player->get_index();
  if (ent_index <= 0 || ent_index >= max_entities) {
    return false;
  }

  record->valid = false;
  record->player = player;
  record->ent_index = ent_index;
  record->sim_time = player->get_simulation_time();
  record->curtime = global_vars->curtime;
  record->origin = player->get_origin();
  record->bounds = get_player_bounds(player, 1.5f);
  record->point_count = 0;
  record->points = {};

  if (!std::isfinite(record->sim_time) || record->sim_time <= 0.0f || !record->bounds.valid) {
    return false;
  }

  const model_t* model = player->get_model();
  if (model == nullptr || model_info == nullptr) {
    return false;
  }

  studio_hdr* hdr = model_info->get_studio_model(model);
  studio_hitbox_set* hitbox_set = hdr != nullptr ? hdr->hitbox_set(player->get_hitbox_set()) : nullptr;
  if (hitbox_set == nullptr) {
    return false;
  }

  matrix_3x4 bone_to_world[128]{};
  if (!player->setup_bones(bone_to_world, 128, 0x100, record->sim_time)) {
    return false;
  }

  const std::uint32_t hitbox_mask = configured_hitbox_mask();
  for (const int hitbox_id : tracked_hitbox_ids) {
    if (!aimbot_hitbox_matches_mask(hitbox_id, hitbox_mask) || hitbox_id >= hitbox_set->num_hitboxes) {
      continue;
    }

    studio_box* hitbox = hitbox_set->hitbox(hitbox_id);
    if (hitbox == nullptr || hitbox->bone < 0 || hitbox->bone >= 128) {
      continue;
    }

    const Vec3 local_center = (hitbox->bbmin + hitbox->bbmax) * 0.5f;
    const Vec3 position = aimbot_transform_point(local_center, bone_to_world[hitbox->bone]);
    if (!aimbot_vec3_is_finite(position)) {
      continue;
    }

    auto& point = record->points[record->point_count];
    point.valid = true;
    point.bone = hitbox->bone;
    point.hitbox = hitbox_id;
    point.priority = aimbot_hitbox_priority(nullptr, player, nullptr, hitbox_id);
    point.position = position;
    ++record->point_count;

    if (record->point_count >= max_points) {
      break;
    }
  }

  record->valid = record->point_count > 0;
  return record->valid;
}

void update_sequences(net_channel* channel)
{
  if (channel == nullptr || global_vars == nullptr) {
    return;
  }

  auto* storage = reinterpret_cast<net_channel_storage*>(channel);
  if (storage->in_sequence_number > g_last_incoming_sequence) {
    g_last_incoming_sequence = storage->in_sequence_number;
    g_sequences.push_front({
      storage->in_reliable_state,
      storage->in_sequence_number,
      global_vars->realtime
    });
  }

  while (g_sequences.size() > 2048) {
    g_sequences.pop_back();
  }
}

void apply_fake_latency(net_channel* channel)
{
  if (channel == nullptr || g_sequences.empty()) {
    return;
  }

  const float target_latency = fake_latency_seconds();
  if (target_latency <= 0.0001f || global_vars == nullptr) {
    return;
  }

  auto* storage = reinterpret_cast<net_channel_storage*>(channel);
  for (const incoming_sequence& sequence : g_sequences) {
    if (global_vars->realtime - sequence.realtime >= target_latency) {
      storage->in_reliable_state = sequence.reliable_state;
      storage->in_sequence_number = sequence.sequence_number;
      break;
    }
  }
}

int send_datagram_hook(net_channel* channel, bf_write* data)
{
  if (g_send_datagram_original == nullptr || channel == nullptr || !should_run_network_state()) {
    return g_send_datagram_original != nullptr ? g_send_datagram_original(channel, data) : 0;
  }

  auto* storage = reinterpret_cast<net_channel_storage*>(channel);
  const int in_sequence_number = storage->in_sequence_number;
  const int in_reliable_state = storage->in_reliable_state;

  apply_fake_latency(channel);
  const int result = g_send_datagram_original(channel, data);

  storage->in_sequence_number = in_sequence_number;
  storage->in_reliable_state = in_reliable_state;
  return result;
}

} // namespace

bool is_enabled()
{
  return config.backtrack.enabled;
}

float fake_latency_seconds()
{
  auto* channel = current_net_channel();
  float real_latency_ms = 0.0f;
  if (channel != nullptr) {
    real_latency_ms = std::clamp(channel->get_latency(flow_outgoing) * 1000.0f, 0.0f, 1000.0f);
  }

  const float requested_ms = std::clamp(config.backtrack.fake_latency_ms, 0.0f, 1000.0f);
  const float clamped_ms = std::clamp(requested_ms, 0.0f, std::max(0.0f, 1000.0f - real_latency_ms));
  return g_latency_ramp * clamped_ms * 0.001f;
}

float interpolation_time()
{
  if (config.backtrack.fake_interp) {
    return window_seconds();
  }

  return local_prediction_interp_time();
}

void on_create_move(user_cmd* user_cmd)
{
  (void)user_cmd;
  install_net_channel_hook();

  auto* channel = current_net_channel();
  if (!should_run_network_state()) {
    g_latency_ramp = 0.0f;
    if (channel == nullptr) {
      g_sequences.clear();
      g_last_incoming_sequence = 0;
    }
    return;
  }

  update_sequences(channel);

  const float tick_interval = global_vars->interval_per_tick > 0.0f
    ? global_vars->interval_per_tick
    : static_cast<float>(TICK_INTERVAL);
  g_latency_ramp = std::min(1.0f, g_latency_ramp + tick_interval);

  if (config.backtrack.fake_interp) {
    channel->set_interpolation_amount(window_seconds());
  }
}

void record_player(Player* player)
{
  if (!should_record() || player == nullptr || entity_list == nullptr) {
    return;
  }

  auto* localplayer = entity_list->get_localplayer();
  if (localplayer == nullptr ||
      player == localplayer ||
      player->get_team() == localplayer->get_team() ||
      player->is_friend() ||
      player->is_ignored()) {
    const int ent_index = player->get_index();
    if (ent_index > 0 && ent_index < max_entities) {
      g_records[ent_index] = {};
    }
    return;
  }

  backtrack_record record{};
  if (!build_record(player, &record)) {
    return;
  }

  auto& history = g_records[record.ent_index];
  history.ent_index = record.ent_index;

  if (history.record_count > 0) {
    const backtrack_record& last = history.records[0];
    if (last.valid &&
        std::fabs(last.sim_time - record.sim_time) <= 0.0001f &&
        aimbot_distance_squared(last.origin, record.origin) <= 0.0001f) {
      return;
    }

    const Vec3 delta = record.origin - last.origin;
    const float delta_sqr = (delta.x * delta.x) + (delta.y * delta.y);
    if (delta_sqr > teleport_distance_sqr()) {
      history.record_count = 0;
    }
  }

  const int shift_count = std::min(history.record_count, max_records - 1);
  for (int index = shift_count; index > 0; --index) {
    history.records[index] = history.records[index - 1];
  }

  history.records[0] = record;
  history.record_count = std::min(history.record_count + 1, max_records);
}

void clear()
{
  g_records = {};
  g_sequences.clear();
  g_last_incoming_sequence = 0;
  g_latency_ramp = 0.0f;
  g_selected_position = std::nullopt;
}

const player_records* records_for_player(Player* player)
{
  if (player == nullptr) {
    return nullptr;
  }

  const int ent_index = player->get_index();
  if (ent_index <= 0 || ent_index >= max_entities) {
    return nullptr;
  }

  return &g_records[ent_index];
}

aimbot_candidate find_hitscan_candidate(Player* localplayer,
  Weapon* weapon,
  Player* player,
  const Vec3& original_view_angles,
  bool preferred)
{
  g_selected_position = std::nullopt;
  if (!is_enabled() ||
      !config.backtrack.aimbot ||
      localplayer == nullptr ||
      weapon == nullptr ||
      player == nullptr ||
      global_vars == nullptr) {
    return {};
  }

  const player_records* history = records_for_player(player);
  if (history == nullptr || history->record_count <= 1) {
    return {};
  }

  const std::uint32_t hitbox_mask = configured_hitbox_mask();
  const float current_sim_time = player->get_simulation_time();
  const Vec3 shoot_pos = localplayer->get_shoot_pos();
  aimbot_candidate best_candidate{};
  int best_priority = INT_MAX;

  for (int record_index = 0; record_index < history->record_count; ++record_index) {
    const backtrack_record& record = history->records[record_index];
    if (!record_valid(record, player)) {
      continue;
    }

    if (std::fabs(record.sim_time - current_sim_time) <= 0.0001f) {
      continue;
    }

    const float sim_age = current_sim_time - record.sim_time;
    if (!std::isfinite(sim_age) || sim_age <= 0.0f || sim_age > window_seconds()) {
      continue;
    }

    for (int point_index = 0; point_index < record.point_count; ++point_index) {
      const backtrack_point& point = record.points[point_index];
      if (!point.valid || !aimbot_hitbox_matches_mask(point.hitbox, hitbox_mask)) {
        continue;
      }

      if (!point_visible(localplayer, record, point)) {
        continue;
      }

      const Vec3 aim_angles = aimbot_calculate_angles_to_position(shoot_pos, point.position);
      const Vec3 view_angles = command_angles(localplayer, aim_angles);
      const float fov = aimbot_calculate_fov(view_angles, original_view_angles);

      if (best_candidate.entity != nullptr &&
          (point.priority > best_priority ||
            (point.priority == best_priority && fov >= best_candidate.fov))) {
        continue;
      }

      aimbot_candidate candidate{};
      candidate.entity = player;
      candidate.player = player;
      candidate.preferred = preferred;
      candidate.bone = point.bone;
      candidate.hitbox = point.hitbox;
      candidate.aim_position = point.position;
      candidate.aim_angles = aim_angles;
      candidate.fov = fov;
      candidate.distance = distance_3d(localplayer->get_origin(), record.origin);
      candidate.health = player->get_health();
      candidate.simulation_time = record.sim_time;
      candidate.tick_count = local_prediction_time_to_ticks(record.sim_time + interpolation_time());
      candidate.command_angles = view_angles;
      candidate.backtrack_mins = record.bounds.mins;
      candidate.backtrack_maxs = record.bounds.maxs;
      candidate.visible = true;
      candidate.backtrack = true;

      best_candidate = candidate;
      best_priority = point.priority;
      g_selected_position = point.position;
    }
  }

  return best_candidate;
}

void draw_visualizer_imgui()
{
  if (!is_enabled() ||
      !config.backtrack.visualizer ||
      engine == nullptr ||
      !engine->is_in_game() ||
      !overlay_projection::begin_frame()) {
    return;
  }

  auto* draw_list = ImGui::GetBackgroundDrawList();
  if (draw_list == nullptr) {
    return;
  }

  const int max_draw_ticks = std::clamp(config.backtrack.visualizer_ticks, 1, max_records);
  for (const player_records& history : g_records) {
    const int draw_count = std::min(history.record_count, max_draw_ticks);
    for (int record_index = 0; record_index < draw_count; ++record_index) {
      const backtrack_record& record = history.records[record_index];
      if (!record.valid || record.point_count <= 0 || !record.points[0].valid) {
        continue;
      }

      Vec3 screen{};
      if (!overlay_projection::world_to_screen(record.points[0].position, &screen)) {
        continue;
      }

      const float alpha = 1.0f - (static_cast<float>(record_index) / static_cast<float>(std::max(draw_count, 1)));
      const int alpha_byte = std::clamp(static_cast<int>(alpha * 210.0f), 35, 210);
      const ImU32 color = IM_COL32(80, 255, 120, alpha_byte);
      draw_list->AddRectFilled(ImVec2(screen.x - 2.0f, screen.y - 2.0f), ImVec2(screen.x + 2.0f, screen.y + 2.0f), color);
    }
  }

  if (g_selected_position) {
    Vec3 screen{};
    if (overlay_projection::world_to_screen(*g_selected_position, &screen)) {
      draw_list->AddRectFilled(
        ImVec2(screen.x - 3.0f, screen.y - 3.0f),
        ImVec2(screen.x + 3.0f, screen.y + 3.0f),
        IM_COL32(255, 70, 70, 240));
    }
  }
}

void install_net_channel_hook()
{
  auto* channel = current_net_channel();
  if (channel == nullptr) {
    return;
  }

  void** vtable = *reinterpret_cast<void***>(channel);
  if (vtable == nullptr) {
    return;
  }

  if (g_hooked_net_channel_vtable == vtable &&
      g_send_datagram_original != nullptr &&
      vtable[net_channel_send_datagram_index] == reinterpret_cast<void*>(send_datagram_hook)) {
    return;
  }

  if (g_hooked_net_channel_vtable != nullptr && g_hooked_net_channel_vtable != vtable) {
    restore_net_channel_hook();
  }

  if (vtable[net_channel_send_datagram_index] == reinterpret_cast<void*>(send_datagram_hook)) {
    g_hooked_net_channel_vtable = vtable;
    return;
  }

  g_send_datagram_original = reinterpret_cast<send_datagram_fn>(vtable[net_channel_send_datagram_index]);
  if (g_send_datagram_original == nullptr) {
    return;
  }

  if (write_to_table(vtable, net_channel_send_datagram_index, reinterpret_cast<void*>(send_datagram_hook))) {
    g_hooked_net_channel_vtable = vtable;
  } else {
    g_send_datagram_original = nullptr;
  }
}

void restore_net_channel_hook()
{
  if (g_hooked_net_channel_vtable == nullptr || g_send_datagram_original == nullptr) {
    g_hooked_net_channel_vtable = nullptr;
    g_send_datagram_original = nullptr;
    return;
  }

  if (g_hooked_net_channel_vtable[net_channel_send_datagram_index] == reinterpret_cast<void*>(send_datagram_hook)) {
    write_to_table(
      g_hooked_net_channel_vtable,
      net_channel_send_datagram_index,
      reinterpret_cast<void*>(g_send_datagram_original));
  }

  g_hooked_net_channel_vtable = nullptr;
  g_send_datagram_original = nullptr;
}

} // namespace backtrack
