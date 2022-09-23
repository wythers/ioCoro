/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

/**
 * @file i dont really want to provide non-ioCoroSyscall read and write, because it
 * is not iocoro or more broadly say, a modern C++ programming design idea to
 * force users to MUST do, so left some space and told the users SHOULD DO, instead of MUST DO
 */

#pragma once

#include "socket.hpp"

#include <poll.h>

namespace ioCoro {

/**
 * @note using the non-ioCoroSyscall guarantees nothing...
 */
static int
blocking_do(int fd, int event);

ssize_t
Read(Socket sock, void* buf, ssize_t len);

ssize_t
Write(Socket sock, void const* buf, ssize_t len);

} // namespace ioCoro
