#pragma once

#include "object_pool.hpp"

namespace ioCoro {

inline SocketImpl*
Alloc(ObjectPool& pool)
{
  return pool.allocate();
}

inline void
Dealloc(ObjectPool& pool, SocketImpl* p)
{
  pool.deallocate(p);
}

template<typename T, IsCoroHandler H, IsSocketType S, typename... Last>
inline constexpr auto*
Acquire(H&& h, S&& s, Last&&... last)
{
  return new (s.GetData().Task)
    T{ forward<H>(h), forward<S>(s), forward<Last>(last)... };
}

template<typename T, typename... Last>
inline constexpr auto*
Alloc(Last&&... last)
{
  return new T{ forward<Last>(last)... };
}

template<typename T>
inline constexpr void
Dealloc(T* p)
{
  delete p;
}

} // namespace ioCoro