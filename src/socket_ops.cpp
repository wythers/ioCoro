#include "socket_ops.hpp"

using namespace ioCoro;

static int
ioCoro::blocking_do(int fd, int event)
{
  pollfd fds;
  fds.fd = fd;
  fds.events = event;
  fds.revents = 0;

  return ::poll(&fds, 1, -1);
}

ssize_t
ioCoro::Read(Socket sock, void* buf, ssize_t len)
{
  for (;;) {
    ssize_t ret = ::read(sock.GetFd(), buf, len);

    if (ret >= 0)
      return ret;

    if (ret == -1 && errno != errors::try_again) {
      sock.UpdateState();
      return -1;
    }

    errno = 0;

    if (blocking_do(sock.GetFd(), POLLIN) < 0) {
      sock.UpdateState();
      return -1;
    }
  }

  return -1;
}

ssize_t
ioCoro::Write(Socket sock, void const* buf, ssize_t len)
{
  for (;;) {
    ssize_t ret = ::send(sock.GetFd(), buf, len, MSG_MORE | MSG_NOSIGNAL);

    if (ret >= 0)
      return ret;

    if (ret == -1 && errno != errors::try_again) {
      sock.UpdateState();
      return -1;
    }

    errno = 0;

    if (blocking_do(sock.GetFd(), POLLOUT) < 0) {
      sock.UpdateState();
      return -1;
    }
  }

  return -1;
}