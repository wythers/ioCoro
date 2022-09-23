/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "iocoro_syscall_impl.hpp"
#include <string_view>
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
 *      ssize_t ret = co_await ioCoroRead(sock, buf, num);
 * @arg sock, a Socket type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size you want to read
 * @return how many bytes are received
 *
 * @note ioCoro-context guarantees that all data is received under normal
 * conditions, otherwise the socket status is changed to reflect an error
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
 * @cond must be in Coroutine to call the syscall
 * @code
 *      ssize_t ret = co_await ioCoroWrite(sock, buf, num);
 * @arg sock, a Socket type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size you want to write
 * @return how many bytes are sent
 *
 * @note ioCoro-context guarantees that all data is sent under normal
 * conditions, otherwise the socket status is changed to reflect an error
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
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroConnect(sock, ip, port);
 * @arg sock, a Socket type
 * @arg ip, char const*, the server end address
 * @arg port, int, the server end port
 * @return void
 *
 * @note ioCoro-context guarantees that the Conncet will finish under normal
 * conditions, otherwise the socket status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroConnect : ioCoroSyscall
{
  bool await_suspend(std::coroutine_handle<> h);

  ioCoroConnect(Socket& inS, char const* inAddr, int inPort)
    : ioCoroSyscall{}
    , ip(inAddr)
    , port(inPort)
    , m_s(inS)
  {
  }

  char const* ip;
  int port;
  Socket& m_s;
};

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      auto [ret, idx] = co_await ioCoroRead(sock, buf, num, delim);
 * @arg sock, a Socket type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size you want to read
 * @arg delim, char const*, the terminate string
 * @return
 *  @arg ret, how many bytes are received
 *  @arg idx, the position of the delim in the buffer
 *
 * @note ioCoro-context guarantees that all data is received OR meets the
 * terminate string(delim) under normal conditions, otherwise the socket status
 * is changed to reflect an error
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

  pair<ssize_t, size_t> await_resume()
  {
    return { total,
             static_cast<char const*>(pos) - static_cast<char const*>(start) };
  }

  Socket& m_s;
  void* buf;
  ssize_t len;
  char const* delim;

  int offset;
  void const* pos;
  void* start;
  ssize_t total;
};

template<typename S>
concept ServerEntrychecker = requires()
{
  {
    S::Passive(Socket{})
    } -> std::same_as<IoCoro<void>>;
};

template<typename S, typename... Args>
concept ClientEntrychecker = requires(Args... args)
{
  {
    S::Active(Socket{}, args...)
    } -> std::same_as<IoCoro<void>>;
};

} // namespace ioCoro