/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "iocoro_syscall.hpp"

namespace ioCoro {

template<IsServiceType Service>
class Server : public SeviceModelBase
{
public:
  Server() = delete;

  Server(char const* ip, int port)
    : m_ip(ip)
    , m_port(port)
  {
    m_tasks.Push(Alloc<PollOperation>(
      m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));
  }

  /**
   * @brief the default ip address is localhost, the same as Server(nullptr,
   * port);
   */
  Server(int port)
    : m_port(port)
  {
    m_tasks.Push(Alloc<PollOperation>(
      m_reactor, m_tasks, m_timer_holders, m_tasks.m_numm));
  }

  /**
   * @brief for performance, if user have a certain estimate of the load, should
   * transmit to ioCoro-context through this interface
   *
   * @ingroup user-context for server end
   */
  void Reserver(uint num) { m_objects.reserver(num); }

  /**
   * @brief if everything is fine, run the server for the service. but there is
   * a point needed to note that once running starts, its either forever or
   * crashed. the server is not like the client end that ioCoro-context
   * guarantees recycling source, every task submitted was finished. to put it
   * simply because the server end is passive, how many clients want to link is
   * unknown so no way to talk 'finish or join' at any time. if you want to stop
   * the server, just SGIKILL the process holding the server, then the
   * OS-context makes everything fine, and coffee tasting, emm... a beautiful
   * day.
   *
   * @ingroup user-context for server end
   */
  void Run()
  {
    static_assert(ServerEntrychecker<Service>,
                  "the Server Entry parameter error.");

    Socket s_accept = AcceptOperation::AcceptInit(*this, m_ip, m_port);
    auto& hook = s_accept.GetData();

    hook.Seed = Alloc<AcceptOperation>(&Service::Passive, s_accept);

    m_tasks.Start();
  }

private:
  char const* m_ip{};

  int m_port{};
};

} // namespace ioCoro
