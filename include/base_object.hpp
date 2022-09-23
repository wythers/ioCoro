/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "default_args.hpp"
#include "dummy.hpp"
#include "object_pool.hpp"
#include "operation.hpp"
#include "reactor.hpp"
#include "thread_pool.hpp"
#include "timer_queue.hpp"

namespace ioCoro {

/**
 * @brief the ioCoro-context core class holding the total ioCoro-context
 */
class SeviceModelBase
{
  /**
   * @note similar to OS, any context-switch from user-context to ioCoro-context
   * requires permission. the difference is that OS-context is supported by
   * hardware. so any user customized iocoroSyscall needs to be registered here
   */

  Register_System_Interactive_Unit Socket;
  Register_System_Interactive_MultiUnit Timer;

  Register_System_Buildin_Call AcceptOperation;
  Register_System_Buildin_Call PollOperation;
  Register_System_Buildin_Call ReadOperation;
  Register_System_Buildin_Call ReadUntilOperation;
  Register_System_Buildin_Call WriteOperation;

  Register_System_Call ioCoroRead;
  Register_System_Call ioCoroReadUntil;
  Register_System_Call ioCoroWrite;
  Register_System_Call ioCoroConnect;

protected:
  Socknum_t m_sock_num{};

  ObjectPool m_objects{};

  Timers_t m_timer_holders{};

  Joinable_t m_join{};

  Reactor m_reactor{};

  TaskPool m_tasks{};
};

using Ios = SeviceModelBase;

} // namespace ioCoro
