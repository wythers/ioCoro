#pragma once

#include "iocoro_syscall.hpp"

namespace ioCoro {

ssize_t
ioCoroCompletedRead::await_resume()
{
  m_s.ShutdownRead();
  return total;
}

ssize_t
ioCoroCompletedWrite::await_resume()
{
  m_s.ShutdownWrite();
  return total;
}

ioCoroConnect
ioCoroReconnect(Socket& inS, char const* inHost)
{
  inS.Refresh();
  return { inS, inHost };
}

#ifdef NEED_IOCORO_TIMER
template<InvokedAndBoolReturn T, typename Rep, typename Period>
bool
ioCoroSuspendFor<T, Rep, Period>::await_suspend(std::coroutine_handle<> h)
{
  Timer tmp(
    [=, this] {
      flag = func();
      h();
    },
    m_s);

  tmp.Detach();
  tmp.After(elapse);

  return true;
}
#endif

pair<ssize_t, size_t>
ioCoroReadUntil::await_resume()
{
  return { total,
           static_cast<char const*>(pos) - static_cast<char const*>(start) };
}

} // namespace ioCoro