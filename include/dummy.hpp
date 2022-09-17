/**
 * Sometimes we need to transfer more straight messages to the user in a more
 * readable way, so dummy.hpp created.
 */
#pragma once

/**
 *
 */
#define Register_System_Call friend struct
#define Register_System_Buildin_Call Register_System_Call

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
