#pragma once

#include <coroutine>
#include <mutex>
#include <stdio.h>

namespace ioCoro {

namespace CoroDetails {

template<typename T>
struct IoCoro
{
  struct promise
  {

    std::suspend_always initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }
    void unhandled_exception() { printf("coro boom\n"); }

    std::suspend_never yield_value(T* inOs)
    {
      //    io_context = inOs;
      return {};
    }

    IoCoro get_return_object()
    {
      return { std::coroutine_handle<promise>::from_promise(*this) };
    }

    void return_void() {}
  };

  IoCoro(std::coroutine_handle<promise> inH)
    : m_h(inH)
  {
  }

  operator std::coroutine_handle<>() { return m_h; }

  using promise_type = promise;

  std::coroutine_handle<promise> m_h;
};

} // namespace CoroDetails

using namespace CoroDetails;

} // namespace ioCoro