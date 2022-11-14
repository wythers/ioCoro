/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "syscall.hpp"

namespace ioCoro {

template<HasServerEntry Service>
class Server : public SeviceModelBase
{
  void Init(char const* host);
  Socket AcceptInit(char const* ip, int port);

public:
  Server() = delete;
  Server(const Server&) = delete;

  /**
   * @code
   *    Server<service> server{"localhost:80"};
   *    Server<service> server{":80"};
   *    Server<service> server{127.0.0.x:80};
   *    Server<service> server{x.x.x.x:p};
   */
  Server(char const* host);

  /**
   * @code
   *    Server<service> server{"localhost:80", n};
   *    Server<service> server{":80", n};
   *    Server<service> server{127.0.0.x:80, n};
   *    Server<service> server{x.x.x.x:p, n};
   */
  Server(char const* host, uint threads)
    : SeviceModelBase{ threads }
  {
  }

  /**
   * @brief for performance, if user have a certain estimate of the load, as a
   * hint, should tell to ioCoro-context through this interface
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
   * the server, just SIGKILL the process holding the server, then the
   * OS-context makes everything fine, and coffee tasting, emm... a beautiful
   * day.
   *
   * @note the @args all must have copy semantics, if some dont you may consider
   * using std::ref to wrap some args, or pass ptrs.
   * 
   * @code for example:
   * 
   *  std::mutex mtx{};
   * 
   *  struct Service
   *  {
   *      static IoCoro Passive(Socket streaming, std::mutex* mtx)
   *      {
   *          ...
   *      }
   *  };
   * 
   *  ...
   *    ioCoro::Server<Service> server{":1024"};
   *    ...
   *    
   *    server.Run(&mtx);
   *    
   *
   * @ingroup user-context for server end
   */
  template<typename... Args>
  void Run(Args&&... args);

private:
  char m_ip[INET_ADDRSTRLEN]{ LOCALHOST };

  int m_port{};
};

} // namespace ioCoro

#include "iocoro_server.ipp"
