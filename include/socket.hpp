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

  template<CanBeInvoked>
  friend struct unique_stream;

public:
  Socket(Ios& ios);

  Socket(Ios& ios, SocketImpl& impl, int fd, Normal);

  Socket(Ios& ios, SocketImpl& impl, int fd, Special);

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
  inline explicit operator bool() const noexcept;

  inline void ClearState() const noexcept;

  inline int StateCode() const noexcept;

  inline std::string const ErrorMessage() const noexcept;

  inline void UpdateState() const noexcept;

  inline void UpdateState(int idx) const noexcept;

  inline void ClosedState() const noexcept;

  /**
   * @brief a set of getting IoCoro-context partial data API
   * @ingroup user-context
   */
public:
  inline int GetFd() const noexcept;

  inline Ios& GetContext() noexcept;

  inline SocketImpl& GetData() noexcept;

  /**
   * @brief a deconstructed flag API
   * @ingroup user-context
   *
   * @note A direct call is not recommended, A better way is like
   * @code:
   *        unique_stream cleanup(stream, stream, stream, ...);
   *        or
   *        unique_stream cleanup([]{
   *             ...;
   *        }, stream, stream, stream, ...);
   *
   * @note by the way, why does name the destructor Unhide, emm... the story is
   * that one of the ioCoro design ideas is every socket(stream) should be
   * distributed from ioCoro-context, once it happened, that means the
   * Socket(Stream) is transparent to ioCoro-context right now. but the user
   * been got the Socket(stream) finished his work, then he should UNHIDE the
   * 'magic' to let the ioCoro-context know "oh, i get a new guy."
   */
public:
  void Unhide();

  /**
   * @brief a set of internal FD control API
   * @ingroup user-context
   */
public:
  inline void ShutdownRead() const noexcept;

  inline void ShutdownWrite() const noexcept;

  inline void ShutdownReadAndWrite() const noexcept;

  /**
   * @brief an API for close
   * @ingroup user-context
   *
   * @note check whether the Stream(Socket) is closed, you can do like
   * @code
   *      if (stream)
   *      {
   *        if (stream.StateCode() = errors::socke_closed)
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
  inline void Close();

private:
  SocketImpl* m_object_ptr{};

  SeviceModelBase* m_ios{};

  int m_fd_copy{};

  /**
   * in non-iocoro-context, accessing the State is not thread safe
   */
  mutable SocketState m_state{};

  Socket* m_next{};
};

using Stream = Socket;

} // namespace ioCoro

#include "socket.ipp"