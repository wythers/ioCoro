/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "concepts.hpp"
#include "socket.hpp"
#include "timer.hpp"

namespace ioCoro {

struct _empty_call
{
  constexpr void operator()() noexcept {}
};

/**
 * @brief the RAII guard for Socket type
 *
 * @code:
 *        unique_socket cleanup([]{
 *          ...
 *        }, sock, sock, sock, ...);
 *
 * @ingroup user-context
 */
template<CanBeInvoked F = _empty_call>
struct unique_socket
{
  unique_socket() = delete;
  unique_socket(unique_socket&) = delete;
  unique_socket& operator=(unique_socket const&) = delete;

  template<typename... Args>
  constexpr void helper(Socket& s, Args&&... args)
  {
    s.m_next = sock;
    sock = &s;

    helper(forward<Args>(args)...);
  }

  constexpr void helper()
  {}

  template<AreAllSockets... Args>
  unique_socket(F&& f, Args&&... args)
    : sock(nullptr)
    , func(forward<F>(f))
  {
    helper(forward<Args>(args)...);
  }

  template<AreAllSockets... Args>
  unique_socket(Socket& s, Args&&... args)
    : sock(nullptr)
    , func(F{})
  {
    helper(s, forward<Args>(args)...);
  }

  unique_socket(unique_socket&& m)
  {
    sock = m.sock;
    func = move(m.func);
  }

  ~unique_socket()
  {
    func();
    
    while (sock)
    {
      sock->Unhide();
      sock = sock->m_next;
    }
  }

  Socket* sock;

  F func;
};

/**
 * @brief the guard for Timer
 *
 * @code:
 *        DeadLine line([]{
 *          ...
 *        }, sock, 1s);
 *
 * @ingroup user-context
 */
template<CanBeInvoked F>
struct DeadLine : Timer<F>
{
  DeadLine() = delete;
  DeadLine(DeadLine const&) = delete;
  DeadLine const& operator=(DeadLine const&) = delete;

  template<typename Rep, typename Period>
  DeadLine(F&& f, Socket& s, std::chrono::duration<Rep, Period> const& elapse)
    : Timer<F>(forward<F>(f), s)
  {
    Timer<F>::Acquire();
    Timer<F>::After(elapse);
  }

  ~DeadLine()
  {
    Timer<F>::Release();
  }
};

} // namespace ioCoro