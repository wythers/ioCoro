#pragma once

#include "default_args.hpp"
#include "error.hpp"

#include <atomic>
#include <fcntl.h>
#include <mutex>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

using std::atomic;
using std::error_code;
using std::mutex;
using std::system_error;

using Socknum_t = atomic<ulong>;
using Joinable_t = atomic<bool>;

namespace ioCoro {

class Reactor
{
public:
  Reactor();

  void Shutdown()
  {
    uint64_t counter(1UL);
    ::write(m_interrupter, &counter, sizeof(uint64_t));

    m_shutdown = true;
  }

  bool IsClosed() { return m_shutdown; }

  int GetFd() { return m_epoll_fd; }

private:
  int m_epoll_fd{};

  int m_interrupter{};

  atomic<bool> m_shutdown{ false };
};

} // namespace ioCoro