#include "socket.hpp"

using namespace ioCoro;

Socket::Socket(Ios& ios)
  : m_ios(&ios)
{
  int fd = 0;
  fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

  int ret = 0;
  if (ret < 0)
    m_state = update_error();
  else {
    m_object_ptr = Alloc(m_ios->m_objects);
    m_object_ptr->m_fd = fd;
    m_fd_copy = fd;
    static_cast<Operation>(m_object_ptr->Ops) =
      Operation{ &SocketImpl::ReactOperation::perform, nullptr };

    ret = 0;
    epoll_event ev{};
    ev.events = ACTIVE;
    ev.data.ptr = static_cast<void*>(&(m_object_ptr->Ops));
    ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0)
      m_state = update_error();

    Hide();
    m_ios->m_sock_num.fetch_add(1, rx);
  }
}

Socket::Socket(Ios& ios, SocketImpl& impl, Normal)
  : m_object_ptr(&impl)
  , m_ios(&ios)
  , m_fd_copy(impl.m_fd)
  , m_state{}
{
  epoll_event ev{};

  ev.events = PASSIVE;
  ev.data.ptr = static_cast<void*>(&(m_object_ptr->Ops));

  int ret = 0;
  ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, m_fd_copy, &ev);
  if (ret < 0)
    m_state = update_error();

  Hide();
  m_ios->m_sock_num.fetch_add(1, rx);
}

Socket::Socket(Ios& ios, SocketImpl& impl, Special)
  : m_object_ptr(&impl)
  , m_ios(&ios)
  , m_fd_copy(impl.m_fd)
  , m_state{}
{
  epoll_event ev{};
  ev.events = NEUTRAL;
  ev.data.ptr = static_cast<void*>(&(m_object_ptr->Op));

  int ret = 0;
  ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, m_fd_copy, &ev);
  if (ret < 0)
    m_state = update_error();

  Hide();
  m_ios->m_sock_num.fetch_add(1, rx);
}

Socket::~Socket()
{
  if (m_object_ptr && !rx_load(m_object_ptr->m_hided)) {
    int fd = m_fd_copy;
    Dealloc(m_ios->m_objects, m_object_ptr);
    epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_DEL, fd, 0);

    ::close(fd);

    if (m_ios->m_sock_num.fetch_sub(1, rx) == 1 && m_ios->m_join.load(rx))
      m_ios->m_join.notify_one();
  }
}

bool
Socket::Read(void*& buf, ssize_t& len, ssize_t& total)
{
  ssize_t ret = 0;
  for (;;) {
    ret = ::read(m_fd_copy, buf, len);

    if (ret == len) {
      total += ret;
      return false;
    }

    if (ret == 0) {
      return false;
    }

    if (ret == -1) {
      if (errno == errors::try_again) {
        errno = 0;
        return true;
      } else if (errno != errors::interrupted) {
        m_state = update_error();
        errno = 0;
        return false;
      }
    }

    errno = 0;
    buf = static_cast<char*>(buf) + ret;
    total += ret;
    len -= ret;
  }
}

bool
Socket::Write(void const*& buf, ssize_t& len, ssize_t& total)
{
  ssize_t ret = 0;
  for (;;) {
    ret = ::send(m_fd_copy, buf, len, MSG_MORE | MSG_NOSIGNAL);

    if (ret == len)
    {
      total += len;
      return false;
    }

    if (ret == -1) {
      if (errno == errors::try_again) {
        errno = 0;
        return true;
      } else if (errno != errors::interrupted) {
        m_state = update_error();
        errno = 0;
        return false;
      }
    }

    errno = 0;
    buf = static_cast<char const*>(buf) + ret;
    total += ret;
    len -= ret;
  }
}