#include "timer_queue.hpp"

using namespace ioCoro;

void
Timer_queue::Push(Operation* op, std::chrono::microseconds const& elapse)
{
  TimePoint cur = std::chrono::system_clock::now();
  cur += elapse;

  lock_guard<mutex> locked{ m_mtx };
  m_timers.emplace(std::move(cur), op);
}

void
Timer_queue::Push(Operation* op, TimePoint const& tp)
{
  lock_guard<mutex> locked{ m_mtx };
  m_timers.emplace(tp, op);
}

std::pair<Op_queue<Operation>, int>
Timer_queue::Batch()
{
  Op_queue<Operation> expired{};
  TimePoint cur = std::chrono::system_clock::now();
  int num{};

  {
    lock_guard<mutex> locked{ m_mtx };
    for (;;) {
      if (m_timers.empty()) {
        if (is_joinable.load(rx)) {
          is_joinable.store(false, rx);
          is_joinable.notify_one();
        }
        break;
      }

      auto const& [p, f] = m_timers.top();
      if (!rx_load(is_rightnow) && p > cur)
        break;

      expired.PushBack(f);
      ++num;
      m_timers.pop();
    }
  }

  if (rx_load(is_rightnow)) {
    while (!expired.IsEmpty()) {
      auto* tmp = expired.Front();
      expired.PopFront();

      (*tmp)();
    }

    return { {}, 0 };
  }

  return { expired, num };
}