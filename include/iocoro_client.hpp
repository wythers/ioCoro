/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "syscall.hpp"

namespace ioCoro {

template<HasClientEntry Service>
class Client : public SeviceModelBase
{
  template<size_t N, typename... Args>
  constexpr void Wrapper(Socket s, Args&&... args);

public:
  Client();
  Client(uint threads);
  
  Client(const Client&) = delete;

  /**
   * @note the Args... must be consistent with the @arg... of the ioCoro
   * entry(Service::Active(Socket, @arg...))
   *
   * @ingroup user-context for client end
   */
  template<typename... Args>
  void Submit(Args&&... args);

  /**
   * @brief iocoro allows user to submit tasks unrelated to sockets
   *
   * @ingroup user-context for client end
   */
  template<CanBeInvoked F>
  void Submit(F&& func);

  /**
   * @brief iocoro guarantees every task(Timer included) submitted was finished
   * correctly and on time, recycling the total resource and shutdown
   * iocoro-context at final
   *
   * @ingroup user-context for client end
   */
  void Join();

  /**
   * @brief be similar to @interface Join(), but what means "right now"? emm...
   * @interface void JoinRightnow() especially works for Timer, sometimes we
   * need it right now to trigger all Timer launched, and then Join()
   *
   * @ingroup user-context for client end
   */
  void JoinRightNow();
};

} // namespace ioCoro

#include "iocoro_client.ipp"