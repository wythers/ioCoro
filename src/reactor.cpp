#include "reactor.hpp"

using namespace ioCoro;

Reactor::Reactor()
{
  int ret{};
  errno = 0;
  m_epoll_fd = epoll_create(EPOLL_SIZE);
  if (m_epoll_fd < 0)
    throw_exception("epoll_create() failed at reactor");

  ret = fcntl(m_epoll_fd, F_SETFD, FD_CLOEXEC);
  if (ret < 0)
    throw_exception("fcntl() failed at reactor");

  m_interrupter = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (m_interrupter < 0)
    throw_exception("eventfd() failed at reactor");

  epoll_event ev = { 0, { 0 } };
  /**
   * interrupter fd registered in LT model, guarantees Reactor finished
   */
  ev.events = EPOLLIN | EPOLLERR;
  ev.data.ptr = nullptr;
  ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_interrupter, &ev);
  if (ret < 0)
    throw_exception("epoll_ctl() failed at reactor");
}
