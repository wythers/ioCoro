#include "iocoro_syscall.hpp"

using namespace ioCoro;

bool
ioCoroRead::await_suspend(std::coroutine_handle<> h)
{
  ssize_t ret = 0;
  for (;;) {
    ret = ::read(m_s.GetFd(), buf, len);

    if (ret == len) {
      total += ret;
      return false;
    }

    if (ret == 0)
      return false;

    if (ret == -1) {
      if (errno == errors::try_again) {
        errno = 0;
        Ios& ios = m_s.GetContext();
        SocketImpl& impl = m_s.GetData();

        impl.Task =
          Alloc<CompleteOperation<ReadOperation>>(h, m_s, buf, len, total);

        rel_store(impl.Ops.m_to_do, true);

        epoll_event ev{};
        ev.events = PASSIVE;
        ev.data.ptr = static_cast<void*>(&(impl.Ops));

        epoll_ctl(ios.m_reactor.GetFd(), EPOLL_CTL_MOD, m_s.GetFd(), &ev);

        return true;
      } else {
        m_s.UpdateState();
        return false;
      }
    }

    buf = static_cast<char*>(buf) + ret;
    total += ret;
    len -= ret;
  }
}

bool
ioCoroReadUntil::await_suspend(std::coroutine_handle<> h)
{
  ssize_t ret = 0;
  int off = strlen(delim);
  string_view checker;
  size_t position = 0;
  for (;;) {
    ret = ::read(m_s.GetFd(), buf, len);

    if (ret == 0) {
      m_s.UpdateState(errors::at_eof);
      return false;
    }

    if (ret == -1) {
      if (errno == errors::try_again) {
        errno = 0;
        Ios& ios = m_s.GetContext();
        SocketImpl& impl = m_s.GetData();

        impl.Task = Alloc<CompleteOperation<ReadUntilOperation>>(
          h, m_s, buf, len, delim, offset, pos, total);

        rel_store(impl.Ops.m_to_do, true);

        epoll_event ev{};
        ev.events = PASSIVE;
        ev.data.ptr = static_cast<void*>(&(impl.Ops));

        epoll_ctl(ios.m_reactor.GetFd(), EPOLL_CTL_MOD, m_s.GetFd(), &ev);

        return true;
      } else {
        m_s.UpdateState();
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
        m_s.UpdateState(errors::no_buffer_space);

      return false;
    }

    buf = static_cast<char*>(buf) + ret;
    len -= ret;
  }
}

bool
ioCoroWrite::await_suspend(std::coroutine_handle<> h)
{
  int ret = 0;
  for (;;) {
    /**
     * Signal PIPE is a bad thing, it can make the IoCoro-context collapse, we
     * must hide the signal, meanwhile reflect this error and leave it to the
     * user to decide how to do
     */
    ret = send(m_s.GetFd(), buf, len, MSG_MORE | MSG_NOSIGNAL);

    if (ret == len) {
      total += ret;
      return false;
    }

    if (ret == -1) {
      if (errno == errors::try_again) {
        errno = 0;
        Ios& ios = m_s.GetContext();
        SocketImpl& impl = m_s.GetData();

        impl.Task =
          Alloc<CompleteOperation<WriteOperation>>(h, m_s, buf, len, total);

        rel_store(impl.Ops.m_to_do, true);

        epoll_event ev{};
        ev.events = ACTIVE;
        ev.data.ptr = static_cast<void*>(&(impl.Ops));

        epoll_ctl(ios.m_reactor.GetFd(), EPOLL_CTL_MOD, m_s.GetFd(), &ev);

        return true;

      } else {
        m_s.UpdateState();
        return false;
      }
    }

    buf = static_cast<char const*>(buf) + ret;
    total += ret;
    len -= ret;
  }
}

bool
ioCoroConnect::await_suspend(std::coroutine_handle<> h)
{
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_port = htons(port);

  int ret = connect(m_s.GetFd(), (struct sockaddr*)&address, sizeof(address));

  if (ret == -1) {

    if (errno == EINPROGRESS) {
      errno = 0;
      Ios& ios = m_s.GetContext();
      SocketImpl& impl = m_s.GetData();

      impl.Task = Alloc<CompleteOperation<ConnectOperation>>(h, m_s);

      rel_store(impl.Ops.m_to_do, true);

      epoll_event ev{};
      ev.events = ACTIVE;
      ev.data.ptr = static_cast<void*>(&(impl.Ops));

      ret = epoll_ctl(ios.m_reactor.GetFd(), EPOLL_CTL_MOD, m_s.GetFd(), &ev);
      if (ret < -1) {
        m_s.UpdateState();
      }

      return true;

    } else {
      m_s.UpdateState();
      return false;
    }
  }

  SocketImpl& impl = m_s.GetData();

  impl.Addr = address;
  impl.Size = sizeof(address);

  return false;
}