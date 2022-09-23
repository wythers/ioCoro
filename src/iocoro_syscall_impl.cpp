#include "iocoro_syscall_impl.hpp"

using namespace ioCoro;

Socket
AcceptOperation::AcceptInit(Ios& ios, char const* ip, int port)
{
  SocketImpl* impl = Alloc(ios.m_objects);
  int fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

  if (fd < 0)
    throw_exception("Accept initialized failed");

  impl->m_fd = fd;
  static_cast<Operation>(impl->Op) = { &SocketImpl::MaintainOperation::perform,
                                       nullptr };
                                      
  struct sockaddr_in Addr{};
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

  Socket tmp{ ios, *impl, Socket::Special{} };
  return tmp;
}

void
AcceptOperation::operator()()
{
  for (;;) {
    sockaddr_in address{};
    socklen_t len = sizeof(address);

    int fd = accept4(
      m_sock.GetFd(), (sockaddr*)&address, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (fd == -1) {
      if (errno == errors::try_again) {
        errno = 0;
        return;
      }
    }

    Ios& ios = m_sock.GetContext();
    SocketImpl* impl = Alloc(ios.m_objects);
    impl->m_fd = fd;
    static_cast<Operation>(impl->Ops) = { &SocketImpl::ReactOperation::perform,
                                          nullptr };

    Socket tmp{ ios, *impl, Socket::Normal{} };

    ios.m_tasks.Push(Acquire<BaseOperation>(m_regular_coro(tmp), tmp));
  }
}

void
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

bool
ConnectOperation::perform(MetaOperation* inOp)
{
  auto* p = static_cast<ConnectOperation*>(inOp);
  sockaddr_in address{};
  socklen_t len = sizeof(address);
  int ret = 0;
  ret = getpeername(p->m_s.GetFd(), (struct sockaddr*)(&address), &len);

  if (ret == -1) {
    p->m_s.UpdateState();
  }
  return false;
}

bool
ReadOperation::perform(MetaOperation* inOp)
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

bool
WriteOperation::perform(MetaOperation* inOp)
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

bool
ReadUntilOperation::perform(MetaOperation* inOp)
{
  auto* p = static_cast<ReadUntilOperation*>(inOp);

  if (p->m_s.ReadUntil(p->buf, p->len, p->total, p->delim, p->offset, p->pos)) {
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