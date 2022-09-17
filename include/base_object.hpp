/**
 *
 */
#pragma once

#include "default_args.hpp"
#include "object_pool.hpp"
#include "operation.hpp"
#include "reactor.hpp"
#include "dummy.hpp"
#include "thread_pool.hpp"
#include "timer_queue.hpp"

namespace ioCoro {

class SeviceModelBase
{
  Register_System_Interactive_Unit Socket;
  Register_System_Interactive_MultiUnit Timer;

  Register_System_Buildin_Call AcceptOperation;
  Register_System_Buildin_Call ReadOperation;
  Register_System_Buildin_Call ReadUntilOperation;
  Register_System_Buildin_Call WriteOperation;

  Register_System_Call ioCoroRead;
  Register_System_Call ioCoroReadUntil;
  Register_System_Call ioCoroWrite;
  Register_System_Call ioCoroConnect;

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

  Timers_t m_timer_holders{};

  Socknum_t m_sock_num{};

  Joinable_t m_join{};

  Reactor m_reactor{};

  TaskPool m_tasks{};
};

using Ios = SeviceModelBase;

} // namespace ioCoro
