/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "base_object.hpp"
#include "concepts.hpp"
#include "coroutines.hpp"
#include "default_args.hpp"
#include "reactor.hpp"
#include "socket.hpp"

#include <memory>
#include <tuple>

using std::coroutine_handle;
using std::make_unique;
using std::unique_ptr;
using std::tuple;

namespace ioCoro {

inline static constexpr size_t MaxOfStreamsAskedOnce = 10;

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
  PostOperation(T&& f, TaskPool& inPool)
    : Operation(&perform)
    , func(std::forward<T>(f))
    , tasks(inPool)
  {
    tasks.m_numm.fetch_add(1, rx);
  }

  static void perform(Operation* inOp);

  T func;
  TaskPool& tasks;
};

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

  inline bool operator()();

  /**
   * @note POLL operation implement is at the header file because once we need
   * Timer, the libiocoro does not need rebuilding again, meanwhile,
   * theoretically, this implement will only retain one copy in memory, emm...
   * not bad.
   */
  inline static void perform(Operation* inOp);

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
template<typename Sv, typename... Args>
struct AcceptOperation : Operation
{
  using Coro_t = decltype(Sv::Passive);

  AcceptOperation(Coro_t* incoro, Socket inSo, Args&&... args)
    : Operation{ &perform }
    , m_sock(inSo)
    , m_regular_coro(std::move(incoro))
    , m_args{forward<Args>(args)...}
  {
  }

  template<size_t N, typename... Tps>
  static constexpr void Wrapper(Socket& s, Tps&&... args)
  {
    static_assert(N <= MaxOfStreamsAskedOnce, "beyond the max of streams asked once");

    if constexpr (IsConsistentForServer<Sv, Socket, Tps...>) {
      s.GetContext().m_tasks.Push(
        Acquire<BaseOperation>(Sv::Passive(s, forward<Tps>(args)...), s));
    } else
      Wrapper<N + 1>(s, Socket{ s.GetContext() }, forward<Tps>(args)...);
  }

  template<size_t... Nums>
  struct Helper
  {
    template<typename... Tps>
    static constexpr void scatter(Socket& s, tuple<Tps...>& tp)
    {
      AcceptOperation::Wrapper<1>(s, std::get<Nums>(tp)...);
    }
  };
  
  template<size_t N>
  using Dummy = Helper<__integer_pack(N)...>;

  void operator()();

  static void perform(Operation* inMeta)
  {
    AcceptOperation* p = static_cast<AcceptOperation*>(inMeta);
    (*p)();
  }

  Socket m_sock;

  Coro_t* m_regular_coro;

  tuple<decay_t<Args>...> m_args;
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

#include "iocoro_syscall_impl.ipp"