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

} // namespace ioCoro