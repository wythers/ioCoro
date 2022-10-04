/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

/**
 * @brief user can define THREADS_NUM to control the number of ioCoro workers
 *
 * @note the libiocoro does NOT need to be RECOMPILED, if you redefine the
 * macro.
 */
// #define THREADS_NUM std::thread::hardware_concurrency()*2

/**
 * @brief user can define EPOLL_SIZE to control a hint of epoll
 *
 * @note the libiocoro does NOT need to be RECOMPILED, if you redefine the
 * macro.
 */
// #define EPOLL_SIZE 20000

/**
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro.
 */
#define EPOLL_EVENT_NUM 128

/**
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro.
 */
#define MAX_LISTEN_CONNECT_NUM 4096

/**
 * @note the libiocoro does NOT need to be RECOMPILED, if you redefine the macro
 */
// #define EPOLL_SIZE 20000

/**
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro
 */
#define CACHE_LINE_SIZE 64

/**
 * @note you should not redefine the macro
 *
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro
 */
#define PASSIVE EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET | EPOLLIN

/**
 * @note you should not redefine the macro
 *
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro
 */
#define NEUTRAL EPOLLIN | EPOLLET

/**
 * @note you should not redefine the macro
 *
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro
 */
#define ACTIVE EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLPRI

/**
 * @note the libiocoro does NOT need to be RECOMPILED, if you redefine the
 * macro.
 */
#ifndef HOLDINGTIME
#ifdef NEED_IOCORO_TIMER
#define HOLDINGTIME 0
#else
#define HOLDINGTIME -1
#endif
#endif

/**
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro.
 */
#define DUMPCOLUMN 16

/**
 * @note the libiocoro does need to be RECOMPILED, if you redefine the macro.
 */
#define PAYLOADSIZE 88

/**
 * @note the libiocoro does NOT need to be RECOMPILED, if you redefine the
 * macro.
 */
#define LOCALHOST "127.0.0.1"
