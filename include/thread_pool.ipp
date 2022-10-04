#pragma once

#include "thread_pool.hpp"

namespace ioCoro {

void
TaskPool::Push(Operation* inOp)
{
  {
    lock_guard<mutex> locked(m_mutex);
    m_ops.PushBack(inOp);
  }
  m_condi.notify_one();
}

void
TaskPool::Push(Op_queue<Operation>& inOps, int inNum)
{
  {
    lock_guard<mutex> locked(m_mutex);
    m_ops.PushBack(inOps);
  }

  for (int i = 0; i < inNum; ++i)
    m_condi.notify_one();
}

Operation*
TaskPool::Pop()
{
  Operation* ret{};

  lock_guard<mutex> locked(m_mutex);
  ret = m_ops.Front();
  m_ops.PopFront();
  return ret;
}

bool
TaskPool::Condi_state()
{
  return ((IsStoped()) || (IsStarted() && !m_ops.IsEmpty()));
}

void
TaskPool::StateChange(atomic<bool>& inState, bool inFlag)
{
  rx_store(inState, inFlag);

  m_condi.notify_all();
}

void
TaskPool::Start()
{
  StateChange(started, true);
}

bool
TaskPool::IsStarted()
{
  return rx_load(started);
}

bool
TaskPool::IsStoped()
{
  return rx_load(stoped);
}

void
TaskPool::NotifyAll()
{
  m_condi.notify_all();
}

} // namespace ioCoro