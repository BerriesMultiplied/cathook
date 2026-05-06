/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/automation/navbot/navbot_jobs.hpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#ifndef NAVBOT_JOBS_HPP
#define NAVBOT_JOBS_HPP

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "features/automation/navbot/navbot_hazards.hpp"
#include "features/automation/navbot/navbot_mesh.hpp"
#include "features/automation/navbot/navbot_path.hpp"
#include "features/automation/navbot/navbot_types.hpp"

namespace navbot
{

struct path_job_request
{
  job_handle handle{};
  path_request request{};
  navbot_hazards hazards{};
  cancellation_token token{};
};

struct path_job_result
{
  job_handle handle{};
  path_result result{};
};

class navbot_jobs
{
public:
  ~navbot_jobs();

  void start(const navbot_mesh* mesh, const navbot_hazards* hazards);
  void stop();
  void cancel_generation(uint32_t generation_id);

  [[nodiscard]] job_handle submit_path_request(const path_request& request);
  [[nodiscard]] std::optional<path_job_result> poll_path_result();

private:
  void worker_main();

  const navbot_mesh* mesh_ = nullptr;
  const navbot_hazards* hazards_ = nullptr;

  std::atomic_bool running_ = false;
  std::atomic_uint64_t next_job_id_ = 1;
  std::thread worker_{};
  std::mutex mutex_{};
  std::vector<path_job_request> pending_requests_{};
  std::vector<path_job_result> completed_results_{};
};

} // namespace navbot

#endif
