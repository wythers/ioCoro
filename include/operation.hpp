/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

namespace ioCoro {

/**
 *
 */
struct Operation
{
  typedef void (*Func_type)(Operation*);

  void operator()() { (*func)(this); }

  Operation() = default;
  Operation& operator=(Operation const&) = default;

  Operation(Func_type inF, Operation* inN)
    : func{ inF }
    , next{ inN }
  {
  }

  Operation(Func_type inFunc)
    : func(inFunc)
    , next{ nullptr }
  {
  }

  Func_type func{};

  Operation* next{};
};

/**
 *
 */
struct MetaOperation : Operation
{
  typedef bool (*Meta_type)(MetaOperation*);

  bool operator()() { return (*meta)(this); }

  MetaOperation(Func_type inFunc, Meta_type inMeta)
    : Operation{ inFunc }
    , meta{ inMeta }
  {
  }

  Meta_type meta{};
};

} // namespace ioCoro
