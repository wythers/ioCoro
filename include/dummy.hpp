/**
 * Sometimes we need to transfer more straight messages to the user in a more
 * readable way, or to hide some gory details, so dummy.hpp created.
 *
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 *
 * @file This is an internal header file, included by some ioCoro headers.
 * do not attempt to use it directly.
 */

#pragma once

/**
 *
 */
#define Register_System_Call friend struct

#define Register_System_Buildin_Call Register_System_Call

#define Register_System_Buildin_MultiCall                                      \
  template<typename>                                                           \
  friend class

#define Register_System_MultiCall                                              \
  template<InvokedAndBoolReturn, typename, typename>                           \
  friend class

/**
 *
 */
#define Register_System_Interactive_Unit friend class
#define Register_System_Interactive_MultiUnit                                  \
  template<typename>                                                           \
  friend class

/**
 *
 */
#ifdef NEED_IOCORO_DUMMY
#define Call co_await
#define Exit co_return
#define Yield co_yield
#endif
