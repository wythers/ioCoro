/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include <errno.h>
#include <system_error>

using std::error_code;
using std::system_error;

namespace ioCoro {

inline void
throw_error(error_code& ec, char const* location)
{
  throw system_error{ ec, location };
}

inline void
throw_exception(char const* location)
{
  error_code er{ errno, std::generic_category() };
  throw_error(er, location);
}

inline error_code
update_error()
{
  error_code tmp{ errno, std::generic_category() };
  errno = 0;
  return tmp;
}

inline error_code
update_error(int ec)
{
  return { ec, std::generic_category() };
}

inline bool
operator==(error_code& ec, int err)
{
  return ec.value() == err;
}

inline bool
operator==(int err, error_code& ec)
{
  return ec == err;
}

/**
 * Although forbid triggering signal PIPE, we must leave an error to reflect
 * what happened
 */
class SDstate : public std::error_category
{
public:
  std::string message(int) const override
  {
    return "The socket has been closed";
  }

  char const* name() const noexcept override { return "socket_closed"; }

  virtual ~SDstate() {}
};

using SocketState = error_code;

namespace errors {

enum
{
  access_denied = EACCES,

  address_family_not_supported = EAFNOSUPPORT,

  address_in_use = EADDRINUSE,

  already_connected = EISCONN,

  already_started = EALREADY,

  connection_aborted = ECONNABORTED,

  connection_refused = ECONNREFUSED,

  connection_reset = ECONNRESET,

  bad_descriptor = EBADF,

  fault = EFAULT,

  host_unreachable = EHOSTUNREACH,

  in_progress = EINPROGRESS,

  interrupted = EINTR,

  invalid_argument = EINVAL,

  message_size = EMSGSIZE,

  name_too_long = ENAMETOOLONG,

  network_down = ENETDOWN,

  network_reset = ENETRESET,

  network_unreachable = ENETUNREACH,

  no_descriptors = EMFILE,

  no_buffer_space = ENOBUFS,

  no_memory = ENOMEM,

  no_permission = EPERM,

  no_protocol_option = ENOPROTOOPT,

  no_such_device = ENODEV,

  not_connected = ENOTCONN,

  not_socket = ENOTSOCK,

  operation_aborted = ECANCELED,

  operation_not_supported = EOPNOTSUPP,

  shut_down = ESHUTDOWN,

  timed_out = ETIMEDOUT,

  try_again = EAGAIN,

  would_block = EWOULDBLOCK,

  socket_closed = EPIPE,

  at_eof = EOF
};

} // namespace errors

} // namespace ioCoro