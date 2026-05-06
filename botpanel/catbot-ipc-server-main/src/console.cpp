#include "core/ipc/ipc_shared.hpp"

#include "json.hpp"

#include <csignal>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <vector>

using json = nlohmann::json;

namespace
{

cat_ipc::shared_memory ipc_memory{};
cat_ipc::shared_state* ipc_state = nullptr;
std::unordered_map<std::string, std::function<json(const json&)>> commands{};

[[nodiscard]] auto has_key(const json& object, const std::string& key) -> bool
{
  return object.find(key) != object.end();
}

void replace_string(std::string& input, const std::string& what, const std::string& with_what)
{
  auto index = input.find(what);
  while (index != std::string::npos)
  {
    input.replace(index, what.size(), with_what);
    index = input.find(what, index + with_what.size());
  }
}

void require_connected()
{
  if (ipc_state == nullptr)
  {
    throw std::runtime_error("not connected to ipc server");
  }
}

[[nodiscard]] auto peer_dead(unsigned int id) -> bool
{
  return id >= cat_ipc::max_peers || !cat_ipc::peer_alive(ipc_state->peer_data[id]);
}

[[nodiscard]] auto query_peer(unsigned int id) -> json
{
  require_connected();
  if (id >= cat_ipc::max_peers)
  {
    throw std::out_of_range("peer out of range");
  }

  json result{};
  if (peer_dead(id))
  {
    result["dead"] = true;
    return result;
  }

  const auto& peer = ipc_state->peer_data[id];
  const auto& data = ipc_state->peer_user_data[id];

  result["name"] = std::string(data.name);
  result["friendid"] = data.friendid;
  result["connected"] = data.connected;
  result["heartbeat"] = data.heartbeat;
  result["ts_injected"] = data.ts_injected;
  result["ts_connected"] = data.ts_connected;
  result["ts_disconnected"] = data.ts_disconnected;

  result["accumulated"] = {
    {"kills", data.accumulated.kills},
    {"deaths", data.accumulated.deaths},
    {"score", data.accumulated.score},
    {"shots", data.accumulated.shots},
    {"hits", data.accumulated.hits},
    {"headshots", data.accumulated.headshots},
  };

  result["ingame"] = {
    {"good", data.ingame.good},
    {"kills", data.ingame.kills},
    {"deaths", data.ingame.deaths},
    {"score", data.ingame.score},
    {"shots", data.ingame.shots},
    {"hits", data.ingame.hits},
    {"headshots", data.ingame.headshots},
    {"team", data.ingame.team},
    {"role", data.ingame.role},
    {"life_state", data.ingame.life_state},
    {"health", data.ingame.health},
    {"health_max", data.ingame.health_max},
    {"x", data.ingame.x},
    {"y", data.ingame.y},
    {"z", data.ingame.z},
    {"player_count", data.ingame.player_count},
    {"bot_count", data.ingame.bot_count},
    {"server", std::string(data.ingame.server)},
    {"mapname", std::string(data.ingame.mapname)},
  };

  result["pid"] = peer.pid;
  result["starttime"] = peer.starttime;
  return result;
}

namespace cmd
{

auto exec(const json& args) -> json
{
  require_connected();
  if (!has_key(args, "target"))
  {
    throw std::runtime_error("undefined pid");
  }
  if (!has_key(args, "cmd"))
  {
    throw std::runtime_error("undefined command");
  }

  const auto target_id = args["target"].get<int>();
  if (target_id < 0 || target_id >= static_cast<int>(cat_ipc::max_peers))
  {
    throw std::out_of_range("peer out of range");
  }
  if (peer_dead(static_cast<unsigned int>(target_id)))
  {
    throw std::runtime_error("peer is not connected");
  }

  auto command = args["cmd"].get<std::string>();
  replace_string(command, " && ", " ; ");
  cat_ipc::queue_command(
    ipc_state,
    target_id,
    command.size() >= cat_ipc::command_data_size - 1 ? cat_ipc::commands::execute_client_cmd_long : cat_ipc::commands::execute_client_cmd,
    command);
  return json{};
}

auto exec_all(const json& args) -> json
{
  require_connected();
  if (!has_key(args, "cmd"))
  {
    throw std::runtime_error("undefined command");
  }

  auto command = args["cmd"].get<std::string>();
  replace_string(command, " && ", " ; ");
  cat_ipc::queue_command(
    ipc_state,
    -1,
    command.size() >= cat_ipc::command_data_size - 1 ? cat_ipc::commands::execute_client_cmd_long : cat_ipc::commands::execute_client_cmd,
    command);
  return json{};
}

auto query(const json& args) -> json
{
  require_connected();
  json result{};
  const auto skip_empty = has_key(args, "skipEmpty") && args["skipEmpty"].get<bool>();

  if (has_key(args, "ids"))
  {
    for (const auto& id_json : args["ids"])
    {
      const auto id = id_json.get<unsigned int>();
      if (skip_empty && peer_dead(id))
      {
        continue;
      }
      result[std::to_string(id)] = query_peer(id);
    }
    return result;
  }

  for (auto id = 0u; id < cat_ipc::max_peers; ++id)
  {
    if (skip_empty && peer_dead(id))
    {
      continue;
    }
    result[std::to_string(id)] = query_peer(id);
  }
  return result;
}

auto squery(const json&) -> json
{
  require_connected();
  return json{{"count", ipc_state->peer_count}, {"command_count", ipc_state->command_count}};
}

auto kill(const json& args) -> json
{
  require_connected();
  if (getuid() != 0)
  {
    throw std::runtime_error("kill can only be used as root");
  }
  if (!has_key(args, "pid"))
  {
    throw std::runtime_error("undefined pid");
  }

  const auto id = args["pid"].get<int>();
  if (id < 0 || id >= static_cast<int>(cat_ipc::max_peers))
  {
    throw std::out_of_range("peer out of range");
  }
  if (peer_dead(static_cast<unsigned int>(id)))
  {
    throw std::runtime_error("already dead");
  }

  ::kill(ipc_state->peer_data[id].pid, SIGKILL);
  return json{};
}

auto echo(const json& args) -> json
{
  return json{{"args", args}};
}

auto connect(const json& args) -> json
{
  if (ipc_state != nullptr)
  {
    throw std::runtime_error("already connected");
  }

  if (has_key(args, "server") && args["server"].get<std::string>() != "cathook_followbot_server")
  {
    throw std::runtime_error("custom ipc server names are not supported by this build");
  }

  ipc_memory = cat_ipc::shared_memory::open_client();
  ipc_state = ipc_memory.state();
  return json{};
}

auto disconnect(const json&) -> json
{
  require_connected();
  ipc_memory.close();
  ipc_state = nullptr;
  return json{};
}

} // namespace cmd

} // namespace

int main()
{
  commands["exec"] = &cmd::exec;
  commands["exec_all"] = &cmd::exec_all;
  commands["query"] = &cmd::query;
  commands["kill"] = &cmd::kill;
  commands["echo"] = &cmd::echo;
  commands["connect"] = &cmd::connect;
  commands["disconnect"] = &cmd::disconnect;
  commands["squery"] = &cmd::squery;

  std::cout << json{{"init", std::time(nullptr)}} << std::endl;

  std::string input{};
  while (std::getline(std::cin, input))
  {
    auto cmdid = std::string{"undefined"};
    try
    {
      const auto args = json::parse(input);
      if (!has_key(args, "command"))
      {
        throw std::runtime_error("empty command");
      }
      if (has_key(args, "cmdid"))
      {
        cmdid = args["cmdid"];
      }
      const auto command = args["command"].get<std::string>();
      if (command == "exit" || command == "quit")
      {
        std::cout << json{{"exit", std::time(nullptr)}} << std::endl;
        break;
      }
      if (const auto it = commands.find(command); it != commands.end())
      {
        std::cout << json{{"status", "success"}, {"cmdid", cmdid}, {"result", it->second(args)}} << std::endl;
      }
      else
      {
        throw std::runtime_error("command not found");
      }
    }
    catch (const std::exception& error)
    {
      std::cout << json{{"status", "error"}, {"cmdid", cmdid}, {"error", std::string{error.what()}}} << std::endl;
    }
  }
}
