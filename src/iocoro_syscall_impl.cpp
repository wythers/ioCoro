#include "iocoro_syscall_impl.hpp"

using namespace ioCoro;

Socket
AcceptOperation::AcceptInit(Ios& ios, char const* ip, int port)
{
  SocketImpl* impl = Alloc(ios.m_objects);
  int fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

  if (fd < 0)
    throw_exception("Accept initialized failed");

  impl->m_fd = fd;
  static_cast<Operation>(impl->Op) = { &SocketImpl::MaintainOperation::perform,
                                       nullptr };
  impl->Addr.sin_family = AF_INET;
  if (ip) {
    inet_pton(AF_INET, ip, &(impl->Addr.sin_addr));
  } else {
    impl->Addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  impl->Addr.sin_port = htons(port);

  int ret = 0;
  ret = ::bind(fd, (struct sockaddr*)(&(impl->Addr)), sizeof(impl->Addr));
  if (ret == -1)
    throw_exception("bind failed");

  ret = ::listen(fd, MAX_LISTEN_CONNECT_NUM);
  if (ret == -1)
    throw_exception("listen failed");

  Socket tmp{ ios, *impl, Socket::Special{} };
  return tmp;
}

void
AcceptOperation::operator()()
{
  for (;;) {
    sockaddr_in address{};
    socklen_t len = sizeof(address);

    int fd = accept4(
      m_sock.GetFd(), (sockaddr*)&address, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (fd == -1) {
      if (errno == EAGAIN) {
        errno = 0;
        return;
      }
    }

    Ios& ios = m_sock.GetContext();
    SocketImpl* impl = Alloc(ios.m_objects);
    impl->m_fd = fd;
    static_cast<Operation>(impl->Ops) = { &SocketImpl::ReactOperation::perform,
                                          nullptr };
    impl->Size = sizeof(impl->Addr);
    impl->Addr = address;

    Socket tmp{ ios, *impl, Socket::Normal{} };

    ios.m_tasks.Push(Alloc<BaseOperation>(m_regular_coro(tmp)));
  }
}