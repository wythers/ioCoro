/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "cache.hpp"

#include "default_args.hpp"
#include "mm_order.hpp"
#include "socket_impl.hpp"

#include <memory>
#include <string.h>
#include <utility>

using std::decay_t;
using std::forward;

namespace ioCoro {

class ObjectPool
{
public:
  SocketImpl* allocate()
  {
    return pool.Get();
  }

  void deallocate(SocketImpl* old)
  {
    old->Next = nullptr;
    rx_store(old->m_closed, false);
    rx_store(old->Ops.m_to_do, false);

    pool.Put(old);
  }

  // do noting, but keep it for backward compatibility
  void reserver(int num)
  {
    int _ = num;
  }
  
  ObjectPool(uint n = 16) : pool(n) {}

private:                  
  zeus::Pool<SocketImpl> pool{};
};

} // namespace ioCoro

#include "object_pool.ipp"