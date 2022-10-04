/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "default_args.hpp"
#include "mm_order.hpp"
#include "socket_impl.hpp"

#include <memory>
#include <string.h>
#include <utility>

using std::decay_t;
using std::forward;

namespace ioCoro {

class alignas(CACHE_LINE_SIZE) ObjectPool
{
public:
  SocketImpl* allocate()
  {
    SocketImpl* p = rx_load(m_pool);
    do {
      if (!p) {
        p = new SocketImpl{};
        return p;
      }
    } while (!acq_compare_exchange_weak(m_pool, p, p->Next));

    return p;
  }

  void deallocate(SocketImpl* old)
  {
    //  memset(static_cast<void*>(old), 0, sizeof(SocketImpl));
    old->Next = nullptr;

    while (!rel_compare_exchange_weak(m_pool, old->Next, old))
      ;
  }

  void reserver(int num)
  {
    auto* chunk = new SocketImpl[num]{};

    for (int i = 0; i < num - 1; ++i) {
      chunk[i].Next = chunk + i + 1;
    }

    m_pool = chunk;
  }

  ~ObjectPool()
  {
    while (std::unique_ptr<SocketImpl> tmp{ m_pool.load() })
      m_pool = tmp->Next;
  }

  bool IsEmpty() { return rx_load(m_pool) == nullptr; }

  atomic<SocketImpl*> m_pool{};
};

} // namespace ioCoro

#include "object_pool.ipp"