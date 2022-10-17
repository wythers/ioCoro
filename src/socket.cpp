#include "socket.hpp"

using namespace ioCoro;

Socket::Socket(Ios& ios)
  : m_ios(&ios)
{
  int fd = 0;
  for (;;) {
    fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
      std::this_thread::yield();
    else
      break;
    errno = 0;
  }

  int ret = 0;
  m_object_ptr = Alloc(m_ios->m_objects);
  m_fd_copy = fd;
  static_cast<Operation>(m_object_ptr->Ops) =
    Operation{ &SocketImpl::ReactOperation::perform, nullptr };

  epoll_event ev{};
  ev.events = EPOLLERR | EPOLLHUP | EPOLLPRI;
  ev.data.ptr = static_cast<void*>(&(m_object_ptr->Ops));
  ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, fd, &ev);
  if (ret < 0)
    m_state = update_error();

  m_ios->m_sock_num.fetch_add(1, rx);
}

Socket::Socket(Ios& ios, SocketImpl& impl, int fd, Normal)
  : m_object_ptr(&impl)
  , m_ios(&ios)
  , m_fd_copy(fd)
  , m_state{}
{
  epoll_event ev{};

  ev.events = EPOLLERR | EPOLLHUP | EPOLLPRI;
  ev.data.ptr = static_cast<void*>(&(m_object_ptr->Ops));

  int ret = 0;
  ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, m_fd_copy, &ev);
  if (ret < 0)
    m_state = update_error();

  m_ios->m_sock_num.fetch_add(1, rx);
}

Socket::Socket(Ios& ios, SocketImpl& impl, int fd, Special)
  : m_object_ptr(&impl)
  , m_ios(&ios)
  , m_fd_copy(fd)
  , m_state{}
{
  epoll_event ev{};
  ev.events = NEUTRAL;
  ev.data.ptr = static_cast<void*>(&(m_object_ptr->Op));

  int ret = 0;
  ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, m_fd_copy, &ev);
  if (ret < 0)
    throw_exception("failed to create acceptor");

  m_ios->m_sock_num.fetch_add(1, rx);
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

void
Socket::Refresh()
{
  int tmp{};

  for (;;) {
    tmp = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (tmp < 0)
      std::this_thread::yield();
    else
      break;
    errno = 0;
  }

  errno = 0;
  epoll_event ev{};
  ev.events = EPOLLERR | EPOLLHUP | EPOLLPRI;
  ev.data.ptr = static_cast<void*>(&(m_object_ptr->Ops));

  int ret = epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_ADD, tmp, &ev);

  if (ret != 0)
    m_state = update_error();
  else
    m_state = update_error(0);

  epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_DEL, m_fd_copy, 0);

  /**
   * we must make the SHUTDOWN before the CLOSE, otherwise, the disgusting ABA
   * problem will come.
   */
  m_fd_copy.rx_locked();
  ::close(m_fd_copy);
  m_fd_copy = tmp;
  m_fd_copy.rel_unlocked();
}

void
Socket::Unhide()
{
  Dealloc(m_ios->m_objects, m_object_ptr);

  epoll_ctl(m_ios->m_reactor.GetFd(), EPOLL_CTL_DEL, m_fd_copy, 0);

  /**
   * why is it closed directly instead of locked first? because in design, once
   * unhide means that there is absolutely no escape Timer, unless
   * Timer::detach() is called.
   */
  ::close(m_fd_copy);

  if (m_ios->m_sock_num.fetch_sub(1, rx) == 1) {
    m_ios->m_join.store(true, rx);
    m_ios->m_join.notify_one();
  }
}

bool
Socket::ReadUntil(void*& buf,
                  ssize_t& len,
                  ssize_t& total,
                  char const* delim,
                  int& offset,
                  void const*& pos)
{
  ssize_t ret = 0;
  int off = strlen(delim);
  string_view checker;
  size_t position = 0;

  for (;;) {
    ret = ::read(m_fd_copy, buf, len);

    if (ret == 0) {
      UpdateState(errors::at_eof);
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

    total += ret;
    if (offset)
      checker = { static_cast<char*>(buf) - off + 1,
                  static_cast<size_t>(ret + off - 1) };
    else
      checker = { static_cast<char*>(buf), static_cast<size_t>(ret) };

    ++offset;

    position = checker.find(delim, 0, off);

    if (position != string_view::npos) {
      pos = &checker[position];
      return false;
    }

    if (ret == len) {

      if (position == string_view::npos)
        UpdateState(errors::no_buffer_space);

      return false;
    }

    buf = static_cast<char*>(buf) + ret;
    len -= ret;
  }
}

bool
Socket::Write(void const*& buf, ssize_t& len, ssize_t& total)
{
  ssize_t ret = 0;
  for (;;) {
    ret = ::send(m_fd_copy, buf, len, MSG_MORE | MSG_NOSIGNAL);

    if (ret == len) {
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
