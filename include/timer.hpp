#pragma once

#include "socket.hpp"
#include "timer_queue.hpp"

#include <memory>

using std::unique_ptr;
using std::chrono::microseconds;
using std::chrono_literals::operator""d;
using std::chrono_literals::operator""h;
using std::chrono_literals::operator""min;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""us;

namespace ioCoro {

template<typename>
struct TimerOperation;

template<typename F>
class Timer
{
public:
  Timer() = delete;
  Timer(Timer&) = delete;

  Timer(Socket& refS, F&& f)
    : m_timer_holders(refS.GetContext().m_timer_holders)
		, m_task_pool(refS.GetContext().m_tasks)
    , m_done{ false }
  {
    m_task = Alloc<TimerOperation<decay_t<F>>>(forward<F>(f), m_done);
  }

  ~Timer() { m_done.wait(false, rx); }

public:
	template<typename Rep, typename Period>
	void After(std::chrono::duration<Rep, Period> const& elapse)
	{
		if (elapse <= elapse.zero())
		{
			m_task_pool.Push(m_task);
			return;
		}

		auto transfer = std::chrono::duration_cast<microseconds>(elapse);
		m_timer_holders.Push(m_task, transfer);
	}

	void At(TimePoint const& tp)
	{
		if (tp <= std::chrono::system_clock::now())
		{
			m_task_pool.Push(m_task);
			return;
		}

		m_timer_holders.Push(m_task, tp);
	}

private:
  Timers_t& m_timer_holders;

	TaskPool& m_task_pool;

  Operation* m_task;

  atomic<bool> m_done;
};

template<typename F>
struct TimerOperation : Operation
{
  TimerOperation(F&& f, atomic<bool>& flag)
    : Operation{&perform}
		, task(forward<F>(f))
    , is_done(flag)
  {
  }

  ~TimerOperation()
  {
    rx_store(is_done, true);
    is_done.notify_one();
  }

  static void perform(Operation* inOp)
  {
    unique_ptr<TimerOperation> p{ static_cast<TimerOperation*>(inOp) };

    (p->task)();
  }

  F task;
  atomic<bool>& is_done;
};

} // namespace iocoro