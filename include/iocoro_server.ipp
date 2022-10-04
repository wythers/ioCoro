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
Server<Service>::Server(char const* host)
{
  Init(host);

  m_tasks.Push(
    Alloc<PollOperation>(m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));
}

template<HasServerEntry Service>
void
Server<Service>::Run()
{
  Socket s_accept = AcceptOperation<Service>::AcceptInit(*this, m_ip, m_port);
  auto& hook = s_accept.GetData();

  hook.Seed = Alloc<AcceptOperation<Service>>(&Service::Passive, s_accept);

  m_tasks.Start();
}

} // namespace ioCoro