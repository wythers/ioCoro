/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "base_object.hpp"
#include "coroutines.hpp"
#include "default_args.hpp"
#include "reactor.hpp"
#include "socket.hpp"
#include "concepts.hpp"

#include <memory>

using std::coroutine_handle;
using std::make_unique;
using std::unique_ptr;

namespace ioCoro {

/**
 * @brief NullOperation buildin-iocoroSyscall is the special one, it has two
 * functions: one is to represent of a signal, the other is to as a mm fence for
 * synchronization
 *
 * @ingroup iocoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct NullOperation : Operation
{
  NullOperation()
    : Operation(&perform)
  {
  }

  static void perform(Operation* inOp)
  {
    unique_ptr<NullOperation> p{ static_cast<NullOperation*>(inOp) };
    return;
  }
};

/**
 * @brief BaseOperation builin-iocoroSyscall, as its name, is just used for
 * most initial cases but not all
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct BaseOperation : Operation
{
  BaseOperation(coroutine_handle<> inH, Socket&)
    : Operation{ &perform }
    , m_h(inH)
  {
  }

  static void perform(Operation* inOp)
  {
    auto* p = static_cast<BaseOperation*>(inOp);
    (p->m_h)();
  }

  std::coroutine_handle<> m_h;
};

/**
 * @brief for the client end, post a task unrelated to the socket to
 * ioCoro-context
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
template<CanBeInvoked T>
struct PostOperation : Operation
{
  PostOperation(T&& f, TaskPool& inPool, atomic<bool>& inJoin)
    : Operation(&perform)
    , func(std::forward<T>(f))
    , tasks(inPool)
    , m_joinable(inJoin)
  {
    tasks.m_numm.fetch_add(1, rx);
  }

  static void perform(Operation* inOp);

  T func;
  TaskPool& tasks;
  atomic<bool>& m_joinable;
};

template<CanBeInvoked T>
void
PostOperation<T>::perform(Operation* inOp)
{
  unique_ptr<PostOperation> p{ static_cast<PostOperation*>(inOp) };
  (p->func)();

  if (p->tasks.m_numm.fetch_sub(1, rx) == 1) {
    rx_store(p->tasks.stoped, true);
    p->tasks.stoped.notify_one();
  }
}

/**
 * @brief PollOperation buildin-iocoroSyscall is specail for linux Epoll
 * function and timer
 *
 * @ingroup Iocoro-Context action, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct PollOperation : Operation
{
  PollOperation(Reactor& inRe,
                TaskPool& inPool,
                Timers_t& inTimers,
                atomic<uint>& inNum)
    : Operation{ &perform }
    , m_Re{ inRe }
    , m_tasks{ inPool }
    , m_timers{ inTimers }
    , m_numm{ inNum }
    , m_fd{ m_Re.GetFd() }
  {
    m_numm.fetch_add(1, rx);
  }

  /**
   * POLL operation implement is at the header file, because once we need Timer,
   * the libiocoro dont need rebuilding again
   */
  [[gnu::noinline]] bool operator()()
  {
    epoll_event events[EPOLL_EVENT_NUM];
    int num_events = epoll_wait(m_fd, events, EPOLL_EVENT_NUM, HOLDINGTIME);

    Op_queue<Operation> local_que{};
    int local_num = 0;

    for (int i = 0; i < num_events; ++i) {
      void* p = events[i].data.ptr;

      /**
       *  poll action is closed, back to caller
       */
      if (!p)
        return true;

      ++local_num;
      auto* cluster = static_cast<Operation*>(p);

      cluster->next = nullptr;
      local_que.PushBack(cluster);
    }

#ifdef NEED_IOCORO_TIMER
    if (local_num != 0)
      m_tasks.Push(local_que, local_num);

    Op_queue<Operation> local_timer{};
    this->next = nullptr;
    local_timer.PushBack(this);
    auto [que, n] = m_timers.Batch();
    ++n;
    local_timer.PushBack(que);

    /**
     * too few tasks to process, so waiting for some time slice
     */
    if (local_num == 0 && n == 1)
      std::this_thread::yield();

    m_tasks.Push(local_timer, n);
#else
    this->next = nullptr;
    local_que.PushBack(this);
    ++local_num;
    m_tasks.Push(local_que, local_num);
#endif

    return false;
  }

  static void perform(Operation* inOp);

  Reactor& m_Re;

  TaskPool& m_tasks;

  Timers_t& m_timers;

  atomic<uint>& m_numm;

  int m_fd;
};

/**
 * @brief For the server end, AcceptOperation iocoroSyscall is used only to
 * accept new guys(fds) from OS-Context, then deal with the guys and at last,
 * transmit the armed guys(Sockets) to User-Context
 *
 * @ingroup Iocoro-Context action, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 *
 */
struct AcceptOperation : Operation
{
  typedef IoCoro<void> (*Regular)(Socket);

  AcceptOperation(Regular incoro, Socket inSo)
    : Operation{ &perform }
    , m_sock(inSo)
    , m_regular_coro(incoro)
  {
  }

  void operator()();

  static void perform(Operation* inMeta)
  {
    AcceptOperation* p = static_cast<AcceptOperation*>(inMeta);
    (*p)();
  }

  static Socket AcceptInit(Ios& ios, char const* ip, int port);

  Socket m_sock;

  Regular m_regular_coro;
};

/**
 * @brief as the name description
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct ConnectOperation : MetaOperation
{
  ConnectOperation(Func_type inF, Socket& inS)
    : MetaOperation{ inF, &perform }
    , m_s(inS)
  {
  }

  static bool perform(MetaOperation* inOp);

  Socket& m_s;
};

/**
 * @brief as the name description
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct ReadOperation : MetaOperation
{
  ReadOperation(Func_type inF,
                Socket& inS,
                void*& inBuf,
                ssize_t& inLen,
                ssize_t& inTotal)
    : MetaOperation{ inF, &perform }
    , m_s(inS)
    , buf(inBuf)
    , len(inLen)
    , total(inTotal)
  {
  }

  static bool perform(MetaOperation* inOp);

  Socket& m_s;

  void*& buf;

  ssize_t& len;

  ssize_t& total;
};

/**
 * @brief as the name description
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct WriteOperation : MetaOperation
{
  WriteOperation(Func_type inF,
                 Socket& inS,
                 void const*& inBuf,
                 ssize_t& inLen,
                 ssize_t& inTotal)
    : MetaOperation{ inF, &perform }
    , m_s(inS)
    , buf(inBuf)
    , len(inLen)
    , total(inTotal)
  {
  }

  static bool perform(MetaOperation* inOp);

  Socket& m_s;

  void const*& buf;

  ssize_t& len;

  ssize_t& total;
};

/**
 * @brief as the name description
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
struct ReadUntilOperation : MetaOperation
{
  ReadUntilOperation(Func_type inF,
                     Socket& inS,
                     void*& inBuf,
                     ssize_t& inLen,
                     char const* inDelim,
                     int& inOffset,
                     void const*& inPos,
                     ssize_t& inTotal)
    : MetaOperation{ inF, &perform }
    , m_s(inS)
    , buf(inBuf)
    , len(inLen)
    , delim(inDelim)
    , offset(inOffset)
    , pos(inPos)
    , total(inTotal)
  {
  }

  static bool perform(MetaOperation* inOp);

  Socket& m_s;

  void*& buf;

  ssize_t& len;

  char const* delim;

  int& offset;

  void const*& pos;

  ssize_t& total;
};

/**
 * @brief as the name description
 *
 * @ingroup ioCoro-context, be hidden
 *
 * @note a buildin operation, should not be direct call from user
 */
template<typename MetaOp>
struct CompleteOperation : MetaOp
{
  ~CompleteOperation() {}

  static void perform(Operation* inOp)
  {
    auto* p = static_cast<CompleteOperation*>(inOp);

    (p->h)();
  }

  template<typename... Args>
  CompleteOperation(std::coroutine_handle<> inH, Args&&... args)
    : MetaOp{ &perform, forward<Args>(args)... }
    , h(inH)
  {
  }

  std::coroutine_handle<> h;
};

} // namespace ioCoro