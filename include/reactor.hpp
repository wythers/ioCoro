/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

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

  ~Reactor() { ::close(m_epoll_fd); }

  void Shutdown()
  {
    uint64_t counter(1UL);
    ::write(m_interrupter, &counter, sizeof(uint64_t));
  }

  int GetFd() { return m_epoll_fd; }

private:
  int m_epoll_fd{};

  int m_interrupter{};
};

} // namespace ioCoro