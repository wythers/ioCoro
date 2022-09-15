#pragma once

#include "concepts.hpp"
#include "default_args.hpp"
#include "mm_order.hpp"
#include "socket_impl.hpp"

#include <string.h>
#include <utility>

using std::decay_t;
using std::forward;

namespace ioCoro {

class ObjectPool;
class Socket;

inline SocketImpl*
Alloc(ObjectPool&);

inline void
Dealloc(ObjectPool&, SocketImpl*);

template<typename T, IsCoroHandler H, IsSocketType S, typename... Last>
inline constexpr auto*
Acquire(H&& h, S&& s, Last&&... last)
{
  return new (s.GetData().Ops.payload)
    T{ forward<H>(h), forward<S>(s), forward<Last>(last)... };
}

template<typename T, typename... Last>
inline constexpr auto*
Alloc(Last&&... last)
{
  return new T{ forward<Last>(last)... };
}

template<typename T>
inline constexpr auto*
Alloc()
{
  return new T{};
}

template<typename T>
inline constexpr void
Dealloc(T* p)
{
  delete p;
}

class alignas(CACHE_LINE_SIZE) ObjectPool
{
public:
  SocketImpl* allocate()
  {
    SocketImpl* p = rx_load(m_pool);
    {
      if (!p)
        return Alloc<SocketImpl>();
    }
    while (!acq_compare_exchange_weak(m_pool, p, p->Next))
      ;

    return p;
  }

  void deallocate(SocketImpl* old)
  {
    //  memset(static_cast<void*>(old), 0, sizeof(SocketImpl));

    rx_store(old->m_hided, true);
    rx_store(old->Ops.m_to_do, false);
    old->Next = nullptr;

    while (!rel_compare_exchange_weak(m_pool, old->Next, old))
      ;
  }

  bool IsEmpty() { return rx_load(m_pool) == nullptr; }

  atomic<SocketImpl*> m_pool{ nullptr };
};

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

} // namespace ioCoro