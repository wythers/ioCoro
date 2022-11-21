#pragma once

#include "iocoro_server.hpp"

namespace ioCoro {

template<HasServerEntry Service>
void
Server<Service>::Init(char const* host)
{
  string_view checker{ host };
  auto pos = checker.find(":");

  if (pos == 0 || checker.substr(0, pos) == "localhost") {
    m_port = atoi(host + pos + 1);
    return;
  }

  memset(m_ip, 0, INET_ADDRSTRLEN);
  strncpy(m_ip, host, pos);
  m_port = atoi(host + pos + 1);
}

template<HasServerEntry Service>
Socket
Server<Service>::AcceptInit(char const* ip, int port)
{
  SocketImpl* impl = Alloc(m_objects);

  int fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

  if (fd < 0)
    throw_exception("Accept initialized failed");

  static_cast<Operation>(impl->Op) = { &SocketImpl::MaintainOperation::perform,
                                       nullptr };

  struct sockaddr_in Addr
  {};
  Addr.sin_family = AF_INET;
  if (ip) {
    inet_pton(AF_INET, ip, &(Addr.sin_addr));
  } else {
    Addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  Addr.sin_port = htons(port);

  int ret = 0;
  ret = ::bind(fd, (struct sockaddr*)(&(Addr)), sizeof(Addr));
  if (ret == -1)
    throw_exception("bind failed");

  ret = ::listen(fd, MAX_LISTEN_CONNECT_NUM);
  if (ret == -1)
    throw_exception("listen failed");

  Socket tmp{ *this, *impl, fd, Socket::Special{} };
  return tmp;
}

template<HasServerEntry Service>
Server<Service>::Server(char const* host)
: m_host(host)
{
  Init(host);

  m_tasks.Push(
    Alloc<PollOperation>(m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));
}

template<HasServerEntry Service>
Server<Service>::Server(char const* host, uint threads)
    : SeviceModelBase{ threads }
    , m_host(host)
{
  Init(host);

  m_tasks.Push(
    Alloc<PollOperation>(m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));
}

template<HasServerEntry Service>
template<typename... Args>
void
Server<Service>::Run(Args&&... args)
{
  Socket s_accept = AcceptInit(m_ip, m_port);
  auto& hook = s_accept.GetData();

  hook.Seed = Alloc<AcceptOperation<Service, decay_t<Args>...>>(
    &Service::Passive, s_accept, forward<Args>(args)...);

  m_tasks.Start();
}

} // namespace ioCoro