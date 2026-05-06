#include "core/ipc/ipc_shared.hpp"

#include <cstdlib>
#include <iostream>
#include <pthread.h>

int main()
{
  try
  {
    auto memory = cat_ipc::shared_memory::open_client();
    pthread_mutex_unlock(&memory.state()->mutex);
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
