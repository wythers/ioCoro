/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

#include "default_args.hpp"
#include "mm_order.hpp"
#include "operation.hpp"
#include "operation_queue.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <mutex>
#include <netinet/in.h>
#include <new>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using std::atomic;
using std::byte;
using std::launder;
using std::mutex;

namespace ioCoro {

/**
 * the Socket Meta-data Holder
 */
struct SocketImpl
{
  atomic<bool> m_closed{};

  struct ReactOperation : Operation
  {
    byte payload[PAYLOADSIZE]{};

    atomic<bool> m_to_do{};

    SocketImpl* m_next{};

    static void perform(Operation* inBase)
    {
      ReactOperation* p = static_cast<ReactOperation*>(inBase);

      bool flag = true;

      /**
       * An atomic lock indicates some unwelcome guys that i know some
       * interesting things coming but pls look for someone else, This is not
       * your business
       */
      if (!acq_compare_exchange_strong(p->m_to_do, flag, false))
        return;

      auto* meta = launder(reinterpret_cast<MetaOperation*>(p->payload));

      if ((*(meta))())
        return;

      Operation* finish = static_cast<Operation*>(meta);
      (*finish)();
    }

    ReactOperation()
      : Operation{ &perform }
    {
    }
  } Ops{};

#define Next Ops.m_next
#define Task Ops.payload

  /**
   * There is an interesting thing here that em... why not a flag like
   * ReactOperation::m_to_do, or more deeply to say a mutex, in the class??
   */
  struct MaintainOperation : Operation
  {
    Operation* seed{};

    static void perform(Operation* inBase)
    {
      MaintainOperation* p = static_cast<MaintainOperation*>(inBase);

      (*(p->seed))();
    }

    MaintainOperation()
      : Operation{ &perform }
      , seed{}
    {
    }
  } Op{};

#define Seed Op.seed
};

/**
 *
 */
struct fd_t
{
  int fd{};
  atomic<bool> lock{};

  fd_t(int inF = 0)
    : fd(inF)
  {
  }

  operator int() const { return fd; }

  fd_t& operator=(int inf)
  {
    fd = inf;
    return *this;
  }

  fd_t(fd_t const& inF)
    : fd(inF.fd)
    , lock(inF.lock.load(rx))
  {
  }

  void acq_locked()
  {
    bool expect;
    do {
      expect = false;
    } while (!lock.compare_exchange_weak(expect, true, acq, rx));
  }

  void rx_locked()
  {
    bool expect;
    do {
      expect = false;
    } while (!lock.compare_exchange_weak(expect, true, rx));
  }

  void rel_unlocked() { lock.store(false, rel); }

  void rx_unlocked() { lock.store(false, rx); }
};

} // namespace ioCoro