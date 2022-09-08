#pragma once

#include "default_args.hpp"
#include "mm_order.hpp"
#include "socket_impl.hpp"

#include <string.h>
#include <utility>

using std::forward;

namespace ioCoro {

class ObjectPool;

inline SocketImpl*
Alloc(ObjectPool&);

inline void
Dealloc(ObjectPool&, SocketImpl*);

template<typename T, typename... Args>
inline constexpr auto*
Alloc(Args&&... args)
{
  return new T{ forward<Args>(args)... };
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