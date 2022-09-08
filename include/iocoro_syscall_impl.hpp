/**
 *
 */
#pragma once

#include "base_object.hpp"
#include "coroutines.hpp"
#include "default_args.hpp"
#include "reactor.hpp"
#include "socket.hpp"

#include <memory>

using std::coroutine_handle;
using std::make_unique;
using std::unique_ptr;

namespace ioCoro {

/**
 * NullOperation iocoroSyscall is the special one, it has two functions: one is
 * to represent of a signal, the other is to as a mm fence for synchronization
 *
 * @note Iocoro && User action
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
 * BaseOperation iocoroSyscall, as its name, is just used for most initial cases
 * but not all
 *
 * @note Iocoro && User action
 */
struct BaseOperation : Operation
{
  BaseOperation(coroutine_handle<> inH)
    : Operation{ &perform }
    , m_h(inH)
  {
  }

  static void perform(Operation* inOp)
  {
    unique_ptr<BaseOperation> p{ static_cast<BaseOperation*>(inOp) };

    (p->m_h)();
  }

  std::coroutine_handle<> m_h;
};

template<typename T>
struct PostOperation : Operation
{
  PostOperation(T&& f)
    : Operation(&perform)
    , func(std::forward<T>(f))
  {
  }

  static void perform(Operation* inOp)
  {
    unique_ptr<PostOperation> p{ static_cast<PostOperation*>(inOp) };
    (p->func)();
  }

  T func;
};

/**
 * PollOperation iocoroSyscall is specail for linux Epoll function
 *
 * @note Iocoro-Context action, be hidden
 */
struct PollOperation : Operation
{
  PollOperation(Reactor& inRe, TaskPool& inPool)
    : Operation{ &perform }
    , m_Re{ inRe }
    , m_tasks{ inPool }
    , m_fd{ m_Re.GetFd() }
  {
  }

  void operator()()
  {
    epoll_event events[EPOLL_EVENT_NUM];
    int num_events = epoll_wait(m_fd, events, EPOLL_EVENT_NUM, HOLDINGTIME);

    Op_queue<Operation> local_que{};
    int local_num = 1;

    for (int i = 0; i < num_events; ++i) {
      void* p = events[i].data.ptr;

      /**
       *  poll action is closed, back to caller
       */
      if (!p)
        return;

      ++local_num;
      auto* cluster = static_cast<Operation*>(p);

      cluster->next = nullptr;
      local_que.PushBack(cluster);
    }

    this->next = nullptr;
    local_que.PushBack(this);
    m_tasks.Push(local_que, local_num);
  }

  static void perform(Operation* inOp)
  {
    auto* p = static_cast<PollOperation*>(inOp);
    (*p)();
  }

  Reactor& m_Re;

  TaskPool& m_tasks;

  int m_fd;
};

/**
 * For the server end, AcceptOperation iocoroSyscall is used only to accept new
 * guys(fds) from OS-Context, then deal with the guys and at last, transmit the
 * armed guys(Sockets) to User-Context
 *
 * @note Iocoro-Context action, be hidden
 */
struct AcceptOperation : Operation
{
  typedef IoCoro<void> (*Regular)(Socket);

  AcceptOperation(Socket inSo, Regular incoro)
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
 *
 */
struct ConnectOperation : MetaOperation
{
  ConnectOperation(Func_type inF, Socket& inS)
    : MetaOperation{ inF, &perform }
    , m_s(inS)
  {
  }

  static bool perform(MetaOperation* inOp)
  {
    auto* p = static_cast<ConnectOperation*>(inOp);
    sockaddr_in address{};
    socklen_t len = sizeof(address);
    int ret = 0;
    ret = getpeername(p->m_s.GetFd(), (struct sockaddr*)(&address), &len);

    if (ret == -1) {
      p->m_s.UpdateState();
      return false;
    }

    SocketImpl& impl = p->m_s.GetData();

    impl.Addr = address;
    impl.Size = len;

    return false;
  }

  Socket& m_s;
};

/**
 *
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

  static bool perform(MetaOperation* inOp)
  {
    auto* p = static_cast<ReadOperation*>(inOp);

    if (p->m_s.Read(p->buf, p->len, p->total)) {
      SocketImpl& impl = p->m_s.GetData();
      rel_store(impl.Ops.m_to_do, true);

      epoll_event ev{};
      ev.events = PASSIVE;
      ev.data.ptr = static_cast<void*>(&(impl.Ops));

      epoll_ctl(p->m_s.GetContext().m_reactor.GetFd(),
                EPOLL_CTL_MOD,
                p->m_s.GetFd(),
                &ev);

      return true;
    }

    return false;
  }

  Socket& m_s;

  void*& buf;

  ssize_t& len;

  ssize_t& total;
};

/**
 *
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

  static bool perform(MetaOperation* inOp)
  {
    auto* p = static_cast<WriteOperation*>(inOp);

    if (p->m_s.Write(p->buf, p->len, p->total)) {
      SocketImpl& impl = p->m_s.GetData();
      rel_store(impl.Ops.m_to_do, true);

      epoll_event ev{};
      ev.events = ACTIVE;
      ev.data.ptr = static_cast<void*>(&(impl.Ops));

      epoll_ctl(p->m_s.GetContext().m_reactor.GetFd(),
                EPOLL_CTL_MOD,
                p->m_s.GetFd(),
                &ev);

      return true;
    }

    return false;
  }

  Socket& m_s;

  void const*& buf;

  ssize_t& len;

  ssize_t& total;
};

/**
 *
 */
template<typename MetaOp>
struct CompleteOperation : MetaOp
{
  ~CompleteOperation() {}

  static void perform(Operation* inOp)
  {
    unique_ptr<CompleteOperation> p{ static_cast<CompleteOperation*>(inOp) };

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