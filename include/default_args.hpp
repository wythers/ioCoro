#pragma once

/**
 * 
 */
#define THREADS_NUM  4

// std::thread::hardware_concurrency()*2


/**
 * 
 */
#define EPOLL_EVENT_NUM 128


/**
 *  
 */
#define MAX_LISTEN_CONNECT_NUM 4096


/**
 * 
 */
#define EPOLL_SIZE 20000


/**
 * 
 */
#define CACHE_LINE_SIZE 64


/**
 * 
 */
#define PASSIVE EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET  |EPOLLIN 


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
#define HOLDINGTIME -1