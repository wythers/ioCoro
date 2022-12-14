/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "iocoro_syscall_impl.hpp"

#ifdef NEED_IOCORO_TIMER
#include "timer.hpp"
#endif

#include <netdb.h>
#include <string_view>
#include <unordered_map>
#include <utility>

using std::pair;
using std::string_view;

namespace ioCoro {

struct ioCoroSyscall
{
  bool await_ready() { return false; }

  bool await_suspend(std::coroutine_handle<> h) { return true; }

  void await_resume() {}
};

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      ssize_t ret = co_await ioCoroRead(stream, buf, num);
 * @arg stream, a Stream type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size
 * @return how many bytes are received
 *
 * @note ioCoro-context guarantees that all data is received under normal
 * conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroRead : ioCoroSyscall
{

  bool await_suspend(std::coroutine_handle<> h);

  ssize_t await_resume() { return total; }

  ioCoroRead(Socket& inS, void* inBuf, size_t inLen)
    : ioCoroSyscall{}
    , m_s(inS)
    , buf(inBuf)
    , len(inLen)
    , total(0)
  {
  }

  Socket& m_s;
  void* buf;
  ssize_t len;

  ssize_t total;
};

/**
 * @brief ioCoroSyscall
 *
 * @note ioCoroCompletedRead(...) equal to first call ioCoroRead(...) and than
 * sock.ShutDownRead()
 *
 * @ingroup user-context
 */
struct ioCoroCompletedRead : ioCoroRead
{
  ioCoroCompletedRead(Socket& inS, void* inBuf, size_t inLen)
    : ioCoroRead(inS, inBuf, inLen)
  {
  }

  inline ssize_t await_resume();
};

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      ssize_t ret = co_await ioCoroWrite(stream, buf, num);
 * @arg stream, a Stream type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk size
 * @return how many bytes are sent
 *
 * @note ioCoro-context guarantees that all data is sent under normal
 * conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroWrite : ioCoroSyscall
{
  bool await_suspend(std::coroutine_handle<> h);

  ioCoroWrite(Socket& inS, void const* inBuf, size_t inLen)
    : ioCoroSyscall{}
    , m_s(inS)
    , buf(inBuf)
    , len(inLen)
    , total(0)
  {
  }

  ssize_t await_resume() { return total; }

  Socket& m_s;
  void const* buf;
  ssize_t len;

  ssize_t total;
};

/**
 * @brief ioCoroSyscall
 *
 * @note ioCoroCompletedWrite(...) equal to first call ioCoroWrite(...) and than
 * sock.ShutDownWrite()
 *
 * @ingroup user-context
 */
struct ioCoroCompletedWrite : ioCoroWrite
{
  ioCoroCompletedWrite(Socket& inS, void const* inBuf, size_t inLen)
    : ioCoroWrite(inS, inBuf, inLen)
  {
  }

  inline ssize_t await_resume();
};

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroConnect(stream, host);
 * @arg stream, a Stream type
 * @arg host, char const*, the server hostname("x.x.x.x:port" or "domain
 * name:port")
 * @return void
 *
 * @note ioCoro-context guarantees that the Conncetion will be finished under
 * normal conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroConnect : ioCoroSyscall
{
  void waiting(coroutine_handle<> h);

  bool await_suspend(std::coroutine_handle<> h);

  ioCoroConnect(Socket& inS, char const* inHost)
    : ioCoroSyscall{}
    , host(inHost)
    , m_s(inS)
  {
  }

  char const* host;
  Socket& m_s;
};

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroReconnect(stream, host);
 * @arg stream, a Stream type
 * @arg host, char const*, the server hostname("x.x.x.x:port" or "domain
 * name:port")
 * @return void
 *
 * @note ioCoro-context guarantees that the Reconncetion will be finished under
 * normal conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
inline ioCoroConnect
ioCoroReconnect(Socket& inS, char const* inHost);

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroSuspendFor(stream, elapse);
 *      or
 *      co_await ioCoroSuspendFor([]{
 *        ...
 *        return true;
 *      }, stream, elapse);
 * 
 * @arg stream, a Stream type
 * @arg elapse, duration<..., ...>.
 * @return bool, return true by default.
 *
 * @note either the user does not provide a predecessor task, if it is provided,
 * it must have a bool return value ioCoro-context guarantees that the
 * ioCoroSuspendFor will be finished under normal conditions.
 *
 * @ingroup user-context
 */
#ifdef NEED_IOCORO_TIMER
template<InvokedAndBoolReturn T = _empty_call_has_return,
         typename Rep,
         typename Period>
struct ioCoroSuspendFor : ioCoroSyscall
{
  ioCoroSuspendFor(Socket& inS, std::chrono::duration<Rep, Period> const& inE)
    : func(T{})
    , m_s(inS)
    , elapse(inE)
    , flag(true)
  {
  }

  ioCoroSuspendFor(T&& inT,
                   Socket& inS,
                   std::chrono::duration<Rep, Period> const& inE)
    : func(forward<T>(inT))
    , m_s(inS)
    , elapse(inE)
    , flag(true)
  {
  }

  inline bool await_suspend(std::coroutine_handle<> h);

  bool await_resume() { return flag; }

  T func;
  Socket& m_s;
  std::chrono::duration<Rep, Period> const& elapse;

  bool flag;
};
#endif

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      auto [ret, idx] = co_await ioCoroRead(stream, buf, num, delim);
 * @arg stream, a Stream type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size
 * @arg delim, char const*, the terminate string
 * @return
 *  @arg ret, how many bytes are received
 *  @arg idx, the position of the delim in the buffer(chunk)
 *
 * @note ioCoro-context guarantees that all data is received OR meets the
 * terminate string(delim) under normal conditions, otherwise the stream
 * status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroReadUntil : ioCoroSyscall
{
  ioCoroReadUntil(Socket& inS, void* inBuf, ssize_t inLen, char const* inDelim)
    : ioCoroSyscall{}
    , m_s(inS)
    , buf(inBuf)
    , len(inLen)
    , delim(inDelim)
    , offset(0)
    , pos(inBuf)
    , start(inBuf)
    , total(0)
  {
  }

  bool await_suspend(std::coroutine_handle<> h);

  inline pair<ssize_t, size_t> await_resume();

  Socket& m_s;
  void* buf;
  ssize_t len;
  char const* delim;

  int offset;
  void const* pos;
  void* start;
  ssize_t total;
};

} // namespace ioCoro

#include "iocoro_syscall.ipp"