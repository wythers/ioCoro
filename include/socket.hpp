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

private:
  /**
   * @note Iocoro make Listen(...) belong to Iocoro-context action, so user
   * cant use Listen(...) directly
   */
  template<typename... Args>
  constexpr void Listen(Args&&...)
  {
    UpdateState(errors::operation_not_supported);
  }

  /**
   *
   */
  template<typename... Args>
  constexpr void Accept(Args&&...)
  {
    UpdateState(errors::operation_not_supported);
  }

  /**
   *
   */
  template<typename... Args>
  constexpr int Connect(Args&&...)
  {
    UpdateState(errors::operation_not_supported);
    return -1;
  }

  /**
   *
   */
  template<typename... Args>
  void Bind(Args...)
  {
    UpdateState(errors::operation_not_supported);
  }

public:
  /**
   * @note Direct call is not recommended. In principle, read and write
   * operations should be completed by calling ioCoroSyscall
   *
   * @code
   *      co_await ioCoroRead(...);
   *      or
   *      callIoCoroRead(...);
   *
   * @note Why use Bool-returnValue??? because we need hook coroutine local
   * varaible to let Iocoro-context pass some sensitive value, emm.. sounds like
   * using linux syscall, right?
   */
  bool Read(void*& buf, ssize_t& len, ssize_t& total);
  bool ReadUntil(void*& buf,
                 ssize_t& len,
                 ssize_t& total,
                 char const* delim,
                 int& offset,
                 void const*& pos);

  bool Write(void const*& buf, ssize_t& len, ssize_t& total);

public:
  explicit operator bool() noexcept { return static_cast<bool>(m_state); }

  void ClearState() { m_state.clear(); }

  int StateCode() const { return m_state.value(); }

  std::string ErrorMessage() const { return m_state.message(); }

  void UpdateState() { m_state = update_error(); }

  void UpdateState(int idx) { m_state = update_error(idx); }

public:
  int GetFd() const { return m_fd_copy; }

  Ios& GetContext() { return *m_ios; }

  SocketImpl& GetData() { return *m_object_ptr; }

  void Hide() { rx_store(m_object_ptr->m_hided, true); }

  void Unhide() { rx_store(m_object_ptr->m_hided, false); }

  void Close() { rel_store(m_object_ptr->m_hided, false); }

  void ShutdownRead() { ::shutdown(m_fd_copy, SHUT_RD); }

  void ShutdownWrite() { ::shutdown(m_fd_copy, SHUT_WR); }

  Socket() = delete;
  Socket(Socket const&) = default;
  Socket& operator=(Socket const&) = default;

private:
  SocketImpl* m_object_ptr{};

  SeviceModelBase* m_ios{};

  /**
   * Try my best to avoid access real data of socket, copy may be a good choice
   */
  int m_fd_copy{};

  SocketState m_state{};
};

} // namespace ioCoro