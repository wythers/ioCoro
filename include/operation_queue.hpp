/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "concepts.hpp"

#include <iterator>

namespace ioCoro {

template<IsOperationType Op>
class Op_queue
{
public:
  inline void PushBack(Op* inFront, Op* inBack);

  inline void PushBack(Op* inOp);

  template<typename OtherOperation>
  inline void PushBack(Op_queue<OtherOperation>& inOther);

  inline void PopFront();

  inline bool IsEmpty();

  inline Op* Front();
  
  inline  Op* Back();

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

#include "operation_queue.ipp"