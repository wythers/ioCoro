/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

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

#ifdef THREADS_NUM
  TaskPool(int inThreadNum = THREADS_NUM);
#else
  TaskPool(int inThreadNum = std::thread::hardware_concurrency() * 2);
#endif

  void Run();
  ~TaskPool();

  inline void Push(Operation* inOp);

  inline void Push(Op_queue<Operation>& inOps, int inNum);

  inline Operation* Pop();

  inline bool Condi_state();

  inline void StateChange(atomic<bool>& inState, bool inFlag);

  inline void Start();

  inline bool IsStarted();

  inline bool IsStoped();

  inline void NotifyAll();

public:
  atomic<bool> stoped{};
  atomic<uint> m_numm{};

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

  atomic<bool> started{};

  condition_variable m_condi{};

  vector<thread> m_threads{};
};

} // namespace ioCoro

#include "thread_pool.ipp"