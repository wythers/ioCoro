#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>

using std::decay_t;

namespace ioCoro {

// forward dec
class Socket;

template<typename S>
concept IsSocketType =  std::is_same_v<decay_t<S>, Socket>;

template<typename H>
concept IsCoroHandler = std::is_convertible_v<decay_t<H>, std::coroutine_handle<>>;

template<typename T>
concept IsSocketOrCoroHandler = IsSocketType<T> || IsCoroHandler<T>;




} // namespace ioCoro