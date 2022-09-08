/**
 *
 */
#pragma once

#include "default_args.hpp"
#include "object_pool.hpp"
#include "operation.hpp"
#include "reactor.hpp"
#include "thread_pool.hpp"

namespace ioCoro {

class SeviceModelBase
{
  friend class Socket;
  friend struct AcceptOperation;
  friend struct ReadOperation;
  friend struct WriteOperation;
  friend struct ioCoroRead;
  friend struct ioCoroWrite;
  friend struct ioCoroConnect;

  using Socknum_t = atomic<ulong>;
  using Joinable_t = atomic<bool>;

public:
  void Run() { m_tasks.Start(); }

  void Stop()
  {
    m_tasks.Stop();
    m_reactor.Shutdown();
  }

  SeviceModelBase() {}

  ~SeviceModelBase() {}

  void StopReactor() { m_reactor.Shutdown(); }

protected:
  ObjectPool m_objects{};

  Socknum_t m_sock_num{};

  Joinable_t m_join{};

  Reactor m_reactor{};

  TaskPool m_tasks{};
};

using Ios = SeviceModelBase;

} // namespace ioCoro
