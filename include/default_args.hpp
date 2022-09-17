#pragma once

/**
 * 
 */
#ifndef THREADS_NUM
#define THREADS_NUM std::thread::hardware_concurrency()*2
#endif


/**
 * 
 */
#ifndef EPOLL_EVENT_NUM
#define EPOLL_EVENT_NUM 128
#endif


/**
 *  
 */
#ifndef MAX_LISTEN_CONNECT_NUM
#define MAX_LISTEN_CONNECT_NUM 4096
#endif


/**
 * 
 */
#ifndef EPOLL_SIZE
#define EPOLL_SIZE 20000
#endif


/**
 * 
 */
#define CACHE_LINE_SIZE 64


/**
 * 
 */
#define PASSIVE EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET | EPOLLIN 


/**
 * 
 */
#define NEUTRAL EPOLLIN |  EPOLLET 


/**
 * 
 */
#define ACTIVE EPOLLOUT |  EPOLLET | EPOLLERR | EPOLLHUP | EPOLLPRI


/**
 * 
 */
#ifndef HOLDINGTIME
#define HOLDINGTIME 0
#endif


/**
 * 
 */
#define DUMPCOLUMN 16


/**
 * 
 */
#ifndef PAYLOADSIZE
#define PAYLOADSIZE 88
#endif


