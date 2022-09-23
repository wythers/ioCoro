/**
 * @file for cpp20Coroutine, GDB seems to be inadaptability for DEBUG. iocoro lib provides the two
 * functions for you to try hooking some key message
 *
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include "default_args.hpp"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

namespace ioCoro {

void
hex_dump(void* p, size_t len, int column);

/**
 * print the data about SIGSEGV from OS-context
 */
void
dump_regs(void* ucontext);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

inline int
count_carry(int n)
{
  asm("shrl %%eax\n\t"
      "jc 1f\n\t"
      "movl $0, %%eax\n\t"
      "jmp 2f\n\t"
      "1:\n\t"
      "movl $1, %%eax\n\t"
      "2:\n\t"
      :
      : "a"(n));
}

#pragma GCC diagnostic pop

template<size_t N>
inline int
pop_count(int n)
{
  if constexpr (N > 0)
    return pop_count<N - 1>(n >> 1) + count_carry(n);
  else
    return 0;
}

} // namespace ioCoro