#pragma once

#include "iocoro_syscall_impl.hpp"

namespace ioCoro {

template<typename Sv>
  template<size_t N, typename... Args>
  constexpr void
  AcceptOperation<Sv>::Wrapper(Socket s, Args&&... args)
  {
    static_assert(N <= MaxOfStreamsAskedOnce, "beyond the max of streams asked once");

    if constexpr (IsConsistentForServer<Sv, Socket, Args...>) {
      m_sock.GetContext().m_tasks.Push(
        Acquire<BaseOperation>(Sv::Passive(s, forward<Args>(args)...), s));
    } else
      Wrapper<N + 1>(s, Socket{ m_sock.GetContext() }, forward<Args>(args)...);
  }

template<typename Sv>
Socket
AcceptOperation<Sv>::AcceptInit(Ios& ios, char const* ip, int port)
{
  SocketImpl* impl = Alloc(ios.m_objects);

  int fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

  if (fd < 0)
    throw_exception("Accept initialized failed");

  static_cast<Operation>(impl->Op) = { &SocketImpl::MaintainOperation::perform,
                                       nullptr };

  struct sockaddr_in Addr
  {};
  Addr.sin_family = AF_INET;
  if (ip) {
    inet_pton(AF_INET, ip, &(Addr.sin_addr));
  } else {
    Addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  Addr.sin_port = htons(port);

  int ret = 0;
  ret = ::bind(fd, (struct sockaddr*)(&(Addr)), sizeof(Addr));
  if (ret == -1)
    throw_exception("bind failed");

  ret = ::listen(fd, MAX_LISTEN_CONNECT_NUM);
  if (ret == -1)
    throw_exception("listen failed");

  Socket tmp{ ios, *impl, fd, Socket::Special{} };
  return tmp;
}

template<typename Sv>
void
AcceptOperation<Sv>::operator()()
{
  for (;;) {
    sockaddr_in address{};
    socklen_t len = sizeof(address);

    int fd = accept4(
      m_sock.GetFd(), (sockaddr*)&address, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (fd == -1) {
      if (errno == errors::try_again || errno == errors::no_buffer_space) {
        errno = 0;
        return;
      }

      if (errno == errors::no_permission)
        throw_exception("the firewall rules forbid connection");

      continue;
    }

    Ios& ios = m_sock.GetContext();
    SocketImpl* impl = Alloc(ios.m_objects);

    static_cast<Operation>(impl->Ops) = { &SocketImpl::ReactOperation::perform,
                                          nullptr };

    Socket tmp{ ios, *impl, fd, Socket::Normal{} };

    Wrapper<1>(tmp);
  }
}

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

inline bool
PollOperation::operator()()
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

inline void
PollOperation::perform(Operation* inOp)
{
  auto* p = static_cast<PollOperation*>(inOp);
  if ((*p)()) {
    if (p->m_numm.fetch_sub(1, rx) == 1) {
      rx_store(p->m_tasks.stoped, true);
      p->m_tasks.stoped.notify_one();
    }

    Dealloc(p);
  }
}

} // namespace ioCoro