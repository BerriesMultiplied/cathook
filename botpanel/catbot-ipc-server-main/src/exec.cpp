#include "core/ipc/ipc_shared.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void replace_string(std::string& input, const std::string& what, const std::string& with_what)
{
  auto index = input.find(what);
  while (index != std::string::npos)
  {
    input.replace(index, what.size(), with_what);
    index = input.find(what, index + with_what.size());
  }
}

} // namespace

int main(int argc, const char** argv)
{
  if (argc < 3)
  {
    std::cerr << "usage: exec <peer_id> <command...>\n";
    return EXIT_FAILURE;
  }

  int target_id = -1;
  try
  {
    target_id = std::stoi(argv[1]);
  }
  catch (const std::exception& error)
  {
    std::cerr << "invalid peer id: " << error.what() << '\n';
    return EXIT_FAILURE;
  }

  if (target_id < 0 || target_id >= static_cast<int>(cat_ipc::max_peers))
  {
    std::cerr << "invalid peer id: " << target_id << '\n';
    return EXIT_FAILURE;
  }

  std::string command{};
  for (auto index = 2; index < argc; ++index)
  {
    if (!command.empty())
    {
      command.push_back(' ');
    }
    command += argv[index];
  }

  try
  {
    auto memory = cat_ipc::shared_memory::open_client();
    auto* state = memory.state();
    if (!cat_ipc::peer_alive(state->peer_data[target_id]))
    {
      std::cerr << "trying to send command to a dead peer\n";
      return EXIT_FAILURE;
    }

    replace_string(command, " && ", " ; ");
    cat_ipc::queue_command(
      state,
      target_id,
      command.size() >= cat_ipc::command_data_size - 1 ? cat_ipc::commands::execute_client_cmd_long : cat_ipc::commands::execute_client_cmd,
      command);
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
