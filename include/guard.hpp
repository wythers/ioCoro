#pragma once

#include "socket.hpp"

namespace ioCoro {

struct unique_socket : Socket
{
  unique_socket() = delete;
  unique_socket(unique_socket&) = delete;
  unique_socket& operator=(unique_socket const&) = delete;

  unique_socket(Socket const& base)
    : Socket(base)
  {
  }
	
  unique_socket(unique_socket&& m)
  {
    m.Hide();

    static_cast<Socket&>(*this) = static_cast<Socket&>(m);
  }

  ~unique_socket() { Socket::Unhide(); }
};

} // namespace ioCoro