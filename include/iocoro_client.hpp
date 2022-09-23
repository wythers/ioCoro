/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "iocoro_syscall.hpp"

namespace ioCoro {

template<IsServiceType Service>
class Client : public SeviceModelBase
{
public:
  Client()
  {
    m_tasks.Push(Alloc<PollOperation>(
      m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));

    m_sock_num.fetch_add(1, rx);

    m_tasks.Start();
  }

  /**
   * @note the Args... must be consistent with the @arg... of the ioCoro
   * entry(Service::Active(Socket, @arg...))
   *
   * @ingroup user-context for client end
   */
  template<typename... Args>
  void Submit(Args&&... args)
  {
    static_assert(
      ClientEntrychecker<Service, Args...>,
      "the Args... must be consistent with the args of the Client Entry.");

    Socket tmp{ *this };

    m_tasks.Push(Acquire<BaseOperation>(
      Service::Active(tmp, forward<Args>(args)...), tmp));
  }

  /**
   * @brief iocoro allows user to submit tasks unrelated to sockets
   *
   * @ingroup user-context for client end
   */
  template<CanBeInvoked F>
  void Submit(F&& func)
  {
    m_tasks.Push(Alloc<PostOperation<std::decay_t<F>>>(
      (forward<F>(func)), m_tasks, m_join));
  }

  /**
   * @brief iocoro guarantees every task submitted was finished, recycling the
   * total resource and shutdown iocoro-context
   *
   * @ingroup user-context for client end
   */
  void Join()
  {
    if (m_sock_num.fetch_sub(1, rx) != 1)
      m_join.wait(false, rx);

    m_reactor.Shutdown();

    m_tasks.stoped.wait(false, rx);

    m_tasks.NotifyAll();
  }
};

} // namespace ioCoro