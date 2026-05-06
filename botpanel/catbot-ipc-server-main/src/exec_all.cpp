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
  if (argc < 2)
  {
    std::cerr << "usage: exec_all <command...>\n";
    return EXIT_FAILURE;
  }

  std::string command{};
  for (auto index = 1; index < argc; ++index)
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
    replace_string(command, " && ", " ; ");
    cat_ipc::queue_command(
      memory.state(),
      -1,
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
