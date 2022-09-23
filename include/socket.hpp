/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "base_object.hpp"
#include "coroutines.hpp"
#include "default_args.hpp"
#include "error.hpp"
#include "mm_order.hpp"
#include "socket_impl.hpp"

using std::string_view;

namespace ioCoro {

class Socket
{
public:
  enum class Mode : int
  {
    Active,
    Passive
  };

  struct Special
  {};

  struct Normal
  {};

  typedef IoCoro<void> (*Regular)(Socket);

public:
  Socket(Ios& ios);

  Socket(Ios& ios, SocketImpl& impl, Normal);

  Socket(Ios& ios, SocketImpl& impl, Special);

  ~Socket();

  Socket() = default;
  Socket(Socket const&) = default;
  Socket& operator=(Socket const&) = default;

public:
  /**
   * @brief A direct call is not recommended. In principle, read and write
   * operations should be completed by calling ioCoroSyscall
   *
   * @code
   *      co_await ioCoroRead(...);
   *      or
   *      Call ioCoroRead(...);
   *
   * @note Why use Bool-returnValue??? because we need hook coroutine local
   * varaible to let Iocoro-context pass some sensitive value, emm.. sounds like
   * using linux syscall, right?
   *
   * @ingroup user-context && ioCoro-context
   */
  bool Read(void*& buf, ssize_t& len, ssize_t& total);
  bool ReadUntil(void*& buf,
                 ssize_t& len,
                 ssize_t& total,
                 char const* delim,
                 int& offset,
                 void const*& pos);

  bool Write(void const*& buf, ssize_t& len, ssize_t& total);

  /**
   * @brief a set of status API
   * @ingroup user-context
   */
public:
  explicit operator bool() noexcept
  {
    if (rx_load(m_object_ptr->m_closed))
      ClosedState();

    return static_cast<bool>(m_state);
  }

  void ClearState() { m_state.clear(); }

  int StateCode() const { return m_state.value(); }

  std::string ErrorMessage() const { return m_state.message(); }

  void UpdateState() { m_state = update_error(); }

  void UpdateState(int idx) { m_state = update_error(idx); }

  void ClosedState()
  {
    m_state = { errors::socket_closed, SDstate{}};
  }

  /**
   * @brief a set of getting IoCoro-context partial data API
   * @ingroup user-context
   */
public:
  int GetFd() const { return m_fd_copy; }

  Ios& GetContext() { return *m_ios; }

  SocketImpl& GetData() { return *m_object_ptr; }

  /**
   * @brief a set of deconstructed flag API
   * @ingroup user-context
   *
   * @note A direct call is not recommended, A better way is like
   * @code:
   *        unique_socket cleanup(sock);
   *        or
   *        unique_socket cleanup(sock, []{
   *             ...;
   *        });
   */
public:
  void Hide() { rx_store(m_object_ptr->m_hided, true); }

  void Unhide() { rx_store(m_object_ptr->m_hided, false); }

  /**
   * @brief a set of internal FD control API
   * @ingroup user-context
   */
public:
  void ShutdownRead() { ::shutdown(m_fd_copy, SHUT_RD); }

  void ShutdownWrite() { ::shutdown(m_fd_copy, SHUT_WR); }

  void ShutdownReadAndWrite() { ::shutdown(m_fd_copy, SHUT_RDWR); }

  /**
   * @brief an API for close
   * @ingroup user-context
   *
   * @note check whether the socket is closed, you can do like
   * @code
   *      if (sock)
   *      {
   *        if (sock.StateCode() = errors::socke_closed)
   *        {
   *           ...
   *        }
   *      }
   * 
   * @note it needs to be pointed out here that in principle, after each ioCoro
   * system call, the socket status needs to be checked immediately. in
   * addition, each user-defined close checkpoint has the same need.
   */
public:
  void Close()
  {
    rx_store(m_object_ptr->m_closed, true);
    ShutdownReadAndWrite();
  }

private:
  SocketImpl* m_object_ptr{};

  SeviceModelBase* m_ios{};

  /**
   * try my best to avoid access meta data of socket, copy may be a good choice
   */
  int m_fd_copy{};

  /**
   * in non-iocoro-context, accessing the State is not thread safe
   */
  SocketState m_state{};
};

} // namespace ioCoro