#include "iocoro_syscall_impl.hpp"

#include <netdb.h>

using namespace ioCoro;

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