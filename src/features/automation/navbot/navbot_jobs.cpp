/*
/^-----^\   data: 2026-04-30
V  o o  V  file: src/features/automation/navbot/navbot_jobs.cpp
 |  Y  |   autor: pupnoodle
  \ Q /
  / - \
  |    \
  |     \     )
  || (___\====
*/

#include "features/automation/navbot/navbot_jobs.hpp"

#include <chrono>
#include <utility>

#include "games/tf2/sdk/interfaces/global_vars.hpp"

namespace navbot
{

navbot_jobs::~navbot_jobs()
{
  stop();
}

void navbot_jobs::start(const navbot_mesh* mesh, const navbot_hazards* hazards)
{
  stop();

  mesh_ = mesh;
  hazards_ = hazards;
  running_ = true;
  worker_ = std::thread(&navbot_jobs::worker_main, this);
}

void navbot_jobs::stop()
{
  running_ = false;
  if (worker_.joinable())
  {
    worker_.join();
  }

  std::scoped_lock lock(mutex_);
  pending_requests_.clear();
  completed_results_.clear();
}

void navbot_jobs::cancel_generation(uint32_t generation_id)
{
  std::scoped_lock lock(mutex_);
  for (auto& request : pending_requests_)
  {
    if (request.request.generation_id == generation_id)
    {
      request.token.cancel();
    }
  }
}

job_handle navbot_jobs::submit_path_request(const path_request& request)
{
  auto handle = job_handle{
    next_job_id_.fetch_add(1),
    request.generation_id
  };
  auto hazard_snapshot = hazards_ != nullptr ? *hazards_ : navbot_hazards{};

  std::scoped_lock lock(mutex_);
  if (pending_requests_.size() >= 8)
  {
    pending_requests_.erase(pending_requests_.begin());
  }

  pending_requests_.push_back(path_job_request{
    handle,
    request,
    std::move(hazard_snapshot),
    cancellation_token{handle.id, std::make_shared<std::atomic_bool>(false)}
  });

  return handle;
}

std::optional<path_job_result> navbot_jobs::poll_path_result()
{
  std::scoped_lock lock(mutex_);
  if (completed_results_.empty())
  {
    return std::nullopt;
  }

  auto result = completed_results_.front();
  completed_results_.erase(completed_results_.begin());
  return result;
}

void navbot_jobs::worker_main()
{
  while (running_)
  {
    path_job_request request{};
    auto have_request = false;

    {
      std::scoped_lock lock(mutex_);
      if (!pending_requests_.empty())
      {
        request = pending_requests_.front();
        pending_requests_.erase(pending_requests_.begin());
        have_request = true;
      }
    }

    if (!have_request)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      continue;
    }

    if (mesh_ == nullptr)
    {
      continue;
    }

    auto current_time = global_vars != nullptr ? global_vars->curtime : 0.0f;
    auto result = solve_path_request(*mesh_, request.hazards, request.request, request.token, current_time);

    std::scoped_lock lock(mutex_);
    if (completed_results_.size() >= 8)
    {
      completed_results_.erase(completed_results_.begin());
    }
    completed_results_.push_back(path_job_result{request.handle, std::move(result)});
  }
}

} // namespace navbot
