/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

// #include "concepts.hpp"
#include "operation.hpp"
#include "operation_queue.hpp"

#include <chrono>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include <iomanip>

using std::lock_guard;
using std::mutex;
using std::priority_queue;
using std::tuple;
using std::vector;
using std::chrono::time_point;

using TimePoint = time_point<std::chrono::system_clock>;

namespace ioCoro {

/**
 * the std::pair cannot meet the demand, recode the utility.
 */
template<IsOperationType T>
struct Holder
{
  TimePoint m_point;
  T* m_ptr;

  struct Compare
  {
    bool operator()(Holder const& a, Holder const& b) const
    {
      return a.GetPoint() > b.GetPoint();
    }
  };

  TimePoint const GetPoint() const { return m_point; }

  T* const GetPtr() const { return m_ptr; }
};

template<size_t N, typename T>
decltype(auto)
get(ioCoro::Holder<T> const& p)
{
  if constexpr (N == 0)
    return p.GetPoint();
  else
    return p.GetPtr();
}

} // namespace ioCoro

template<typename T>
struct std::tuple_size<ioCoro::Holder<T>>
{
  static constexpr int value = 2;
};

template<size_t I, typename T>
struct std::tuple_element<I, ioCoro::Holder<T>>
{
  using type = T*;
};

template<typename T>
struct std::tuple_element<0, ioCoro::Holder<T>>
{
  using type = TimePoint;
};

namespace ioCoro {

class Timer_queue
{
public:
  Timer_queue() = default;
  Timer_queue(Timer_queue const&) = delete;
  Timer_queue& operator=(Timer_queue&) = delete;

  void Push(Operation* op, std::chrono::microseconds const& elapse)
  {
    TimePoint cur = std::chrono::system_clock::now();
    cur += elapse;

    lock_guard<mutex> locked{ m_mtx };
    m_timers.emplace(std::move(cur), op);
  }

  void Push(Operation* op, TimePoint const& tp)
  {
    lock_guard<mutex> locked{ m_mtx };
    m_timers.emplace(tp, op);
  }

  /**
   * the Batch function is theoretically only one copy kept in memory(or cache??), 
   * so default inline is a better way
   */
  std::pair<Op_queue<Operation>, int> Batch()
  {
    Op_queue<Operation> expired{};
    TimePoint cur = std::chrono::system_clock::now();
    int num{};

    {
      lock_guard<mutex> locked{ m_mtx };
      for (;;) {
        if (m_timers.empty())
          break;

        auto const& [p, f] = m_timers.top();
        if (p > cur)
          break;

        expired.PushBack(f);
        ++num;
        m_timers.pop();
      }
    }
    return { expired, num };
  }

private:
  mutex m_mtx{};

  priority_queue<Holder<Operation>,
                 vector<Holder<Operation>>,
                 Holder<Operation>::Compare>
    m_timers{};
};

using Timers_t = Timer_queue;

} // namespace ioCoro