#pragma once

#include "socket.hpp"

namespace ioCoro {

struct _empty_call
{
  constexpr void operator()() noexcept {}
};

template<typename F = _empty_call>
struct unique_socket
{
  unique_socket() = delete;
  unique_socket(unique_socket&) = delete;
  unique_socket& operator=(unique_socket const&) = delete;

  unique_socket(Socket& base, F&& f)
    : sock(base)
    , func(forward<F>(f))
  {
  }

	unique_socket(Socket& base)
		: sock(base)
		{}

  unique_socket(unique_socket&& m)
  {
    sock = m.sock;
    func = move(m.func);
  }

  ~unique_socket()
  {
    func();
    sock.Unhide();
  }

  Socket& sock;

  F func;
};

} // namespace ioCoro