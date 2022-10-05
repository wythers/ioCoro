/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "socket.hpp"
#include "timer_queue.hpp"

#include <memory>

using std::unique_ptr;
using std::chrono::microseconds;

/**
 *  the order is {day, hour, minute, second, millisecond, and microsecond}
 */
using std::chrono_literals::operator""d;
using std::chrono_literals::operator""h;
using std::chrono_literals::operator""min;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""us;

namespace ioCoro {

template<typename F>
struct TimerOperation : Operation
{
  TimerOperation(F&& f, atomic<bool>& flag);

  ~TimerOperation();

  void operator()();

  static void perform(Operation* inOp)
  {
    unique_ptr<TimerOperation> p{ static_cast<TimerOperation*>(inOp) };

    (*p)();
  }

  F task;
  atomic<bool>& is_done;

  atomic<int>* is_released;
  bool is_detached;

  TaskPool* pool;
};

template<typename F>
class Timer
{
public:
  Timer() = delete;
  Timer(Timer&) = delete;

  Timer(F&& f, Socket& refS);

  ~Timer() { m_done.wait(false, rx); }

public:
  /**
   * @brief the Timer is triggered after some time
   * 
   * @code:
   *      Stream stream;
   *      ...
   *      Timer tm([]{
   *        ...
   *      }, stream);
   *
   *      tm.After(1d) // tm.After(1m) // tm.After(1s) // ... // tm.After(1us)
   *
   * @ingroup user-context
   */
  template<typename Rep, typename Period>
  void After(std::chrono::duration<Rep, Period> const& elapse);

  void At(TimePoint const& tp);

  /**
   * @brief decoupling the Timer and Stream, but the user must guarantees the
   * action of Timer is not related the Stream
   * 
   * @code
   *      Stream stream;
   *      ...
   *      Timer tm([]{
   *        ...
   *      }, stream);
   *
   *      tm.Detach();
   *
   *      tm.After(1d) // tm.After(1m) // tm.After(1s) // ... // tm.After(1us)
   *
   * @ingroup user-context
   */
  void Detach();

  /**
   * @brief sometimes we need such a mechanism that drawing a deadline to ensure
   * the task is finished on time, otherwise the task will be discarded or
   * stoped, etc. the pair of @interface void Acquire() and @interface void
   * Release() is designed for it.
   * 
   * @code
   *      Stream stream;
   *      ...
   *      Timer tm([]{
   *        ...
   *      }, stream, 3s);
   *
   *      // drawing the START of the code block
   *      tm.Acquire();
   *
   *      // The code block must be passed after 3 seconds at the latest,
   *      // otherwise the deadline will be triggered 
   *       tm.After(1d) or tm.After(1m) or tm.After(1s) or ... or tm.After(1us)
   *
   *      ...
   *      // drawing the END of the code block
   *      tm.Release();
   * 
   * @note fortunately, ioCoro provides a @class DeadLine automating to finish the mechanism
   * 
   * @code
   *      ...
   * 
   *      {
   *          DeadLine line([]{
   *            ...
   *          }, stream, 3s);
   * 
   *          ...
   *      }
   * 
   *      ...
   *
   * @ingroup user-context
   */
  void Acquire();
  void Release();

private:
  Timers_t& m_timer_holders;

  TaskPool& m_task_pool;

  Operation* m_task;

  atomic<bool> m_done;

  atomic<int>* m_to_release;
};

} // namespace iocoro


#include "timer.ipp"
