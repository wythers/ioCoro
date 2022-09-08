#pragma once

#include "default_args.hpp"
#include "mm_order.hpp"
#include "operation.hpp"
#include "operation_queue.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using std::atomic;
using std::condition_variable;
using std::lock_guard;
using std::mutex;
using std::thread;
using std::unique_lock;
using std::vector;

namespace ioCoro {

class TaskPool
{
public:
#define m_mutex m_packed.m
#define m_ops m_packed.ops

  TaskPool(int inThreadNum = THREADS_NUM)
  {
    for (int i = 0; i < inThreadNum; ++i) {
      m_threads.emplace_back([this] { Run(); });
    }
  }

  void Run()
  {
    while (!IsStoped()) {

      Operation* task{};

      {
        unique_lock<mutex> locked(m_mutex);
        m_condi.wait(locked, [this] { return Condi_state(); });

        if (IsStoped())
          return;

        task = m_ops.Front();
        m_ops.PopFront();
      }
      /**
       * Condi_state() guarantees that the task must be not NULL.
       */
      (*task)();
    }
  }

  void Push(Operation* inOp)
  {
    {
      lock_guard<mutex> locked(m_mutex);
      m_ops.PushBack(inOp);
    }

    m_condi.notify_one();
  }

  void Push(Op_queue<Operation>& inOps, int inNum)
  {
    {
      lock_guard<mutex> locked(m_mutex);
      m_ops.PushBack(inOps);
    }

    for (int i = 0; i < inNum; ++i)
      m_condi.notify_one();
  }

  Operation* Pop()
  {
    Operation* ret{};

    lock_guard<mutex> locked(m_mutex);
    ret = m_ops.Front();
    m_ops.PopFront();
    return ret;
  }

  inline bool Condi_state()
  {
    return ((IsStoped()) || (IsStarted() && !m_ops.IsEmpty()));
  }

  inline void StateChange(atomic<bool>& inState, bool inFlag)
  {
    rx_store(inState, inFlag);

    m_condi.notify_all();
  }

  inline void Stop()
  {
    StateChange(stoped, true);
  }

  inline void Start()
  {
    StateChange(started, true);
  }

  inline bool IsStarted()
  {
    return rx_load(started);
  }

  inline bool IsStoped()
  {
    return rx_load(stoped);
  }

  ~TaskPool()
  {
    for (auto& t : m_threads)
      t.join();
  }

private:
  /**
   * package this set of data
   */
  struct alignas(CACHE_LINE_SIZE) dummy
  {
    mutex m{};
    Op_queue<Operation> ops{};
  } m_packed{};

#define m_mutex m_packed.m
#define m_ops m_packed.ops

  uint32_t numm{};

  atomic<bool> stoped{};

  atomic<bool> started{};

  condition_variable m_condi{};

  vector<thread> m_threads{};
};

} // namespace ioCoro