#pragma once

#include <iterator>

namespace ioCoro {

template<typename Op>
class Op_queue
{
public:
  void PushBack(Op* inFront, Op* inBack)
  {
    if (!m_front) {
      m_front = inFront;
      m_back = inBack;
      return;
    }

    m_back->next = inFront;
    m_back = inBack;
  }

  void PushBack(Op* inOp) { PushBack(inOp, inOp); }

  template<typename OtherOperation>
  void PushBack(Op_queue<OtherOperation>& inOther)
  {
    Op* inFront = static_cast<Op*>(inOther.Front());
    Op* inBack = static_cast<Op*>(inOther.Back());

    PushBack(inFront, inBack);
  }

  void PopFront()
  {
    if (m_front) {
      m_front = static_cast<Op*>(m_front->next);
      if (m_front == nullptr)
        m_back = nullptr;
    }
  }

  bool IsEmpty() { return m_front == nullptr; }

  Op* Front() { return m_front; }
  Op* Back() { return m_back; }

private:
  struct EndIterator
  {};

  struct Iterator
  {
    using value_type = Op;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    Iterator(Op* inPtr)
      : m_raw(inPtr)
    {
    }

    value_type* operator*() const { return m_raw; }

    Iterator operator++()
    {
      m_raw = static_cast<Op*>(m_raw->next);
      return *this;
    }

    Iterator operator++(int)
    {
      m_raw = static_cast<Op*>(m_raw->next);
      return *this;
    }

    bool operator==(EndIterator) const { return m_raw == nullptr; }

    Op* m_raw;
  };

public:
  Iterator begin() { return Iterator(m_front); }

  EndIterator end() { return {}; }

private:
  Op* m_front{};

  Op* m_back{};
};

} // namespace ioCoro