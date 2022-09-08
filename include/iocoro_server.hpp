#pragma once

#include "iocoro_syscall.hpp"

namespace ioCoro {

template<typename Service>
class Server : public SeviceModelBase
{
public:
  Server() = delete;

  Server(char const* ip, int port)
    : m_ip(ip)
    , m_port(port)
  {
    m_tasks.Push(Alloc<PollOperation>(m_reactor, m_tasks));
  }

  Server(int port)
    : m_port(port)
  {
    m_tasks.Push(Alloc<PollOperation>(m_reactor, m_tasks));
  }

  void Run()
  {
    Socket s_accept = AcceptOperation::AcceptInit(*this, m_ip, m_port);
    auto& hook = s_accept.GetData();

    hook.Seed = Alloc<AcceptOperation>(s_accept, &Service::Passive);

    Ios::Run();
  }

private:
  char const* m_ip{};

  int m_port{};
};

} // namespace ioCoro
