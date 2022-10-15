/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include <concepts>
#include <coroutine>
#include <type_traits>

using std::decay_t;
using std::false_type;
using std::true_type;

namespace ioCoro {

// forward dec
class Socket;
class Operation;

template<typename... Args>
struct __And : false_type
{
};

template<typename... Args>
struct __And<Socket, Args...>
{
  static constexpr bool value = __And<Args...>::value;
};

template<>
struct __And<> : true_type
{
};

template<typename... Args>
concept AreAllSockets = __And<decay_t<Args>...>::value;

template<typename S>
concept IsSocketType = std::is_same_v<decay_t<S>, Socket>;

template<typename H>
concept IsCoroHandler =
  std::is_convertible_v<decay_t<H>, std::coroutine_handle<>>;

template<typename T>
concept IsSocketOrCoroHandler = IsSocketType<T> || IsCoroHandler<T>;

template<typename F>
concept CanBeInvoked = std::is_invocable_v<decay_t<F>>;

template<typename F>
concept InvokedAndBoolReturn = CanBeInvoked<F> && requires(F f)
{
  {
    f()
    } -> std::convertible_to<bool>;
};

/**
 * service restrictions
 */
template<typename Active, typename Passive>
struct ServiceRestriction : std::true_type
{
};

template<typename Entry>
struct EndRestriction : std::true_type
{
};

template<typename S>
concept IsServiceType =
  ServiceRestriction<decltype(S::Active), decltype(S::Passive)>::value;

template<typename S>
concept HasClientEntry = EndRestriction<decltype(S::Active)>::value;

template<typename S>
concept HasServerEntry = EndRestriction<decltype(S::Passive)>::value;

template<typename S, typename... Args>
concept IsConsistentForClient = requires(Args&&... args)
{
  S::Active(forward<Args>(args)...);
};

template<typename S, typename... Args>
concept IsConsistentForServer = requires(Args&&... args)
{
  S::Passive(forward<Args>(args)...);
};

/**
 *
 */
template<typename O>
concept IsOperationType = requires(O o)
{
  static_cast<Operation>(o);
};

/**
 *
 */
template<std::size_t... Nums>
struct ValueList
{
  static constexpr int size = sizeof...(Nums);
  static constexpr int min = 0;
  static constexpr int max = size - 1;
};

template<std::size_t N>
using GetNumbers = ValueList<__integer_pack(N)...>;

} // namespace ioCoro