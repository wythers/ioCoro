#include "iocoro_syscall.hpp"

#include <netdb.h>
#include <unordered_map>

using std::unordered_map;
using namespace ioCoro;

bool
ioCoroRead::await_suspend(std::coroutine_handle<> h)
{
  if (m_s)
    return false;

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

        Acquire<CompleteOperation<ReadOperation>>(h, m_s, buf, len, total);

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
  if (m_s)
    return false;

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

        Acquire<CompleteOperation<ReadUntilOperation>>(
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
  if (m_s)
    return false;

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

        Acquire<CompleteOperation<WriteOperation>>(h, m_s, buf, len, total);

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

void
ioCoroConnect::waiting(coroutine_handle<> h)
{
  Ios& ios = m_s.GetContext();
  SocketImpl& impl = m_s.GetData();

  Acquire<CompleteOperation<ConnectOperation>>(h, m_s);

  rel_store(impl.Ops.m_to_do, true);

  epoll_event ev{};
  ev.events = ACTIVE;
  ev.data.ptr = static_cast<void*>(&(impl.Ops));

  epoll_ctl(ios.m_reactor.GetFd(), EPOLL_CTL_MOD, m_s.GetFd(), &ev);
}

bool
ioCoroConnect::await_suspend(std::coroutine_handle<> h)
{
  if (m_s)
    return false;

  errno = 0;
  /**
   * the ioCoro local host database
   */
  static mutex hostDB_mtx{};
  static std::unordered_map<std::string, pair<sockaddr, socklen_t>> hostDB{};

  int ret{};
  if (std::isalpha(host[0])) {
    pair<sockaddr, socklen_t>* hostinfo{};

    {
      lock_guard<mutex> locked{ hostDB_mtx };
      if (hostDB.count(host))
        hostinfo = &hostDB[host];
    }

    if (hostinfo) {
      ret = ::connect(m_s.GetFd(), &hostinfo->first, hostinfo->second);

      if (ret == 0)
        return false;

      if (ret == -1 && errno == errors::in_progress) {
        errno = 0;
        waiting(h);

        return true;
      }
    }

    auto pos = string_view{ host }.find(":");
    if (pos == string_view::npos) {
      throw system_error
      { 
        { errors::fault, std::generic_category() },
          "the host format error at Client end" 
      };
    }
    std::string name{ host, pos };

    addrinfo hints{};
    addrinfo* result{};

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    for (;;) {
      int s = getaddrinfo(name.data(), host + pos + 1, &hints, &result);
      if (s == 0)
        break;

      if (s != EAI_AGAIN) {
        m_s.UpdateState(errors::network_unreachable);
        return false;
      }
    }

    unique_ptr<addrinfo, void (*)(addrinfo*)> cleanup(
      result, [](addrinfo* rs) { freeaddrinfo(rs); });

    for (addrinfo* p = result; p != nullptr; p = p->ai_next) {

      ret = ::connect(m_s.GetFd(), p->ai_addr, p->ai_addrlen);

      if (ret == 0) {
        {
          lock_guard<mutex> locked{ hostDB_mtx };
          hostDB[host] = { *(p->ai_addr), p->ai_addrlen };
        }

        return false;
      }

      if (ret == -1 && errno == errors::in_progress) {
        {
          lock_guard<mutex> locked{ hostDB_mtx };
          hostDB[host] = { *(p->ai_addr), p->ai_addrlen };
        }

        errno = 0;
        waiting(h);

        return true;
      }
    }

    m_s.UpdateState(errors::host_unreachable);
    return false;

  } else {

    auto pos = string_view{ host }.find(":");
    if (pos == string_view::npos) {
      throw system_error
      { 
        {  errors::fault, std::generic_category() },
          "the host format error at Client end" 
      };
    }

    std::string ip{ host, pos };

    int port = atoi(host + pos + 1);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip.data(), &address.sin_addr);
    address.sin_port = htons(port);

    ret = ::connect(m_s.GetFd(), (struct sockaddr*)&address, sizeof(address));

    if (ret == -1) {

      if (errno == errors::in_progress) {
        errno = 0;
        waiting(h);

        return true;

      } else {
        m_s.UpdateState();
        return false;
      }
    }
  }

  return false;
}
