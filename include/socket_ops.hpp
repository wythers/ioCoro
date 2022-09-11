#pragma once

#include "socket.hpp"

#include <poll.h>

namespace ioCoro {

static int blocking_do(int fd, int event);

ssize_t
Read(Socket sock, void* buf, ssize_t len);

ssize_t
Write(Socket sock, void const* buf, ssize_t len);

} // namespace ioCoro
