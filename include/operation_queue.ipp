#pragma once

#include "operation_queue.hpp"

namespace ioCoro {

template<IsOperationType Op>
void
Op_queue<Op>::PushBack(Op* inFront, Op* inBack)
{
  if (!m_front) {
    m_front = inFront;
    m_back = inBack;
    return;
  }

  m_back->next = inFront;
  m_back = inBack;
}

template<IsOperationType Op>
template<typename OtherOperation>
void
Op_queue<Op>::PushBack(Op_queue<OtherOperation>& inOther)
{
  if (inOther.IsEmpty())
    return;

  Op* inFront = static_cast<Op*>(inOther.Front());
  Op* inBack = static_cast<Op*>(inOther.Back());

  PushBack(inFront, inBack);
}

template<IsOperationType Op>
void
Op_queue<Op>::PopFront()
{
  if (m_front) {
    m_front = static_cast<Op*>(m_front->next);
    if (m_front == nullptr)
      m_back = nullptr;
  }
}

template<IsOperationType Op>
void
Op_queue<Op>::PushBack(Op* inOp)
{
  PushBack(inOp, inOp);
}

template<IsOperationType Op>
bool
Op_queue<Op>::IsEmpty()
{
  return m_front == nullptr;
}

template<IsOperationType Op>
Op*
Op_queue<Op>::Front()
{
  return m_front;
}

template<IsOperationType Op>
Op*
Op_queue<Op>::Back()
{
  return m_back;
}

} // namespace ioCoro