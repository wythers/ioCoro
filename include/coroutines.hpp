/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include <coroutine>
#include <mutex>
#include <stdio.h>

namespace ioCoro {

namespace CoroDetails {

struct IoCoro
{
  struct promise
  {

    std::suspend_always initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }
    void unhandled_exception()
    {
      std::rethrow_exception(std::current_exception());
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