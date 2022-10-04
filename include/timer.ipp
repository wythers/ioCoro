#pragma once

#include "timer.hpp"

namespace ioCoro {

template<typename F>
TimerOperation<F>::TimerOperation(F&& f, atomic<bool>& flag)
  : Operation{ &perform }
  , task(forward<F>(f))
  , is_done(flag)
  , is_released(nullptr)
  , is_detached(false)
  , pool(nullptr)
{
}

template<typename F>
TimerOperation<F>::~TimerOperation()
{
  if (!is_detached) {
    rx_store(is_done, true);
    is_done.notify_one();
  } else if (pool && pool->m_numm.fetch_sub(1, rx) == 1) {
    rx_store(pool->stoped, true);
    pool->stoped.notify_one();
  }
}

template<typename F>
void
TimerOperation<F>::operator()()
{
  if (!is_released) {
    task();
    return;
  }

  if (is_released->fetch_sub(1, rx) == 1) {
    Dealloc(is_released);
    is_detached = true;
    return;
  }

  task();
}

template<typename F>
Timer<F>::Timer(F&& f, Socket& refS)
  : m_timer_holders(refS.GetContext().m_timer_holders)
  , m_task_pool(refS.GetContext().m_tasks)
  , m_done{ false }
  , m_to_release{ nullptr }
{
  static_assert(CanBeInvoked<F>, "the timer second arg must be callably.");

  m_task = Alloc<TimerOperation<decay_t<F>>>(forward<F>(f), m_done);
}

template<typename F>
template<typename Rep, typename Period>
void
Timer<F>::After(std::chrono::duration<Rep, Period> const& elapse)
{
  if (elapse <= elapse.zero()) {
    m_task_pool.Push(m_task);
    return;
  }

  auto transfer = std::chrono::duration_cast<microseconds>(elapse);
  m_timer_holders.Push(m_task, transfer);
}

template<typename F>
void
Timer<F>::At(TimePoint const& tp)
{
  if (tp <= std::chrono::system_clock::now()) {
    m_task_pool.Push(m_task);
    return;
  }

  m_timer_holders.Push(m_task, tp);
}

template<typename F>
inline void
Timer<F>::Detach()
{
  static_cast<TimerOperation<decay_t<F>>*>(m_task)->is_detached = true;
  static_cast<TimerOperation<decay_t<F>>*>(m_task)->pool = &m_task_pool;
  rx_store(m_done, true);

  m_task_pool.m_numm.fetch_add(1, rx);
}

template<typename F>
void
Timer<F>::Acquire()
{
  auto* tmp = Alloc<atomic<int>>(2);

  static_cast<TimerOperation<decay_t<F>>*>(m_task)->is_released = tmp;
  m_to_release = tmp;
}

template<typename F>
void
Timer<F>::Release()
{
  switch (m_to_release->fetch_sub(1, rx)) {
    case 2:
      rx_store(m_done, true);
      break;

    default:
      Dealloc(m_to_release);
  }
}

} // namespace ioCoro