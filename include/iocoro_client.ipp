#pragma once

#include "iocoro_client.hpp"

namespace ioCoro {

template<HasClientEntry Service>
template<size_t N, typename... Args>
constexpr void
Client<Service>::Wrapper(Socket s, Args&&... args)
{
  static_assert(N <= MaxOfStreamsAskedOnce,
                "beyond the max of streams asked once");

  if constexpr (IsConsistentForClient<Service, Socket, Args...>) {
    m_tasks.Push(
      Acquire<BaseOperation>(Service::Active(s, forward<Args>(args)...), s));
  } else
    Wrapper<N + 1>(Socket{ *this }, s, forward<Args>(args)...);
}

template<HasClientEntry Service>
Client<Service>::Client()
{
  m_tasks.Push(
    Alloc<PollOperation>(m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));

  m_sock_num.fetch_add(1, rx);

  m_tasks.Start();
}

template<HasClientEntry Service>
Client<Service>::Client(uint threads)
  : SeviceModelBase{ threads }
{
  m_tasks.Push(
    Alloc<PollOperation>(m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));

  m_sock_num.fetch_add(1, rx);

  m_tasks.Start();
}

template<HasClientEntry Service>
template<typename... Args>
void
Client<Service>::Submit(Args&&... args)
{
  Wrapper<1>(Socket{ *this }, forward<Args>(args)...);
}

template<HasClientEntry Service>
template<CanBeInvoked F>
void
Client<Service>::Submit(F&& func)
{
  m_tasks.Push(
    Alloc<PostOperation<std::decay_t<F>>>(forward<F>(func), this->m_tasks));
}

template<HasClientEntry Service>
void
Client<Service>::Join()
{
  if (m_sock_num.fetch_sub(1, rx) != 1)
    m_join.wait(false, rx);

#ifdef NEED_IOCORO_TIMER
  rx_store(m_timer_holders.is_joinable, true);
  m_timer_holders.is_joinable.wait(true, rx);
#endif

  m_reactor.Shutdown();

  m_tasks.stoped.wait(false, rx);

  m_tasks.NotifyAll();
}

template<HasClientEntry Service>
void
Client<Service>::JoinRightNow()
{
  if (m_sock_num.fetch_sub(1, rx) != 1)
    m_join.wait(false, rx);

#ifdef NEED_IOCORO_TIMER
  rx_store(m_timer_holders.is_joinable, true);
  rx_store(m_timer_holders.is_rightnow, true);
  m_timer_holders.is_joinable.wait(true, rx);
#endif

  m_reactor.Shutdown();

  m_tasks.stoped.wait(false, rx);

  m_tasks.NotifyAll();
}

} // namespace ioCoro