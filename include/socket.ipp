#pragma once

#include "socket.hpp"

namespace ioCoro {

Socket::operator bool() const noexcept
{
  if (rx_load(m_object_ptr->m_closed))
    ClosedState();
  else if (IsKeepAlive() && (m_state.value() == errors::at_eof ||
                            m_state.value() == errors::not_connected))
    UpdateState(errors::timed_out);

  return static_cast<bool>(m_state);
}

void
Socket::Close()
{
  rx_store(m_object_ptr->m_closed, true);
  ShutdownReadAndWrite();
}

void
Socket::ShutdownRead() const noexcept
{
  ::shutdown(m_fd_copy, SHUT_RD);
}

void
Socket::ShutdownWrite() const noexcept
{
  ::shutdown(m_fd_copy, SHUT_WR);
}

void
Socket::ShutdownReadAndWrite() const noexcept
{
  ::shutdown(m_fd_copy, SHUT_RDWR);
}

int
Socket::GetFd() const noexcept
{
  return m_fd_copy;
}

Ios&
Socket::GetContext() noexcept
{
  return *m_ios;
}

SocketImpl&
Socket::GetData() noexcept
{
  return *m_object_ptr;
}

void
Socket::ClearState() const noexcept
{
  m_state.clear();
}

int
Socket::StateCode() const noexcept
{
  return m_state.value();
}

std::string const
Socket::ErrorMessage() const noexcept
{
  return m_state.message();
}

void
Socket::UpdateState() const noexcept
{
  m_state = update_error();
}

void
Socket::UpdateState(int idx) const noexcept
{
  m_state = update_error(idx);
}

void
Socket::ClosedState() const noexcept
{
  m_state = { errors::socket_closed, get_socket_closed_error_category() };
}

void
Socket::KeepAlive() noexcept
{
  m_keepalive = true;
}

bool
Socket::IsKeepAlive() const noexcept
{
  return m_keepalive;
}

} // namespace ioCoro