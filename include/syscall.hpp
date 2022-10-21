#pragma once

#include "concepts.hpp"

namespace ioCoro {

struct Socket;

struct ioCoroSyscall;

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      ssize_t ret = co_await ioCoroRead(stream, buf, num);
 * @arg stream, a Stream type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size
 * @return how many bytes are received
 *
 * @note ioCoro-context guarantees that all data is received under normal
 * conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroRead;

/**
 * @brief ioCoroSyscall
 *
 * @note ioCoroCompletedRead(...) equal to first call ioCoroRead(...) and than
 * sock.ShutDownRead()
 *
 * @ingroup user-context
 */
struct ioCoroCompletedRead;

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      ssize_t ret = co_await ioCoroWrite(stream, buf, num);
 * @arg stream, a Stream type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk size
 * @return how many bytes are sent
 *
 * @note ioCoro-context guarantees that all data is sent under normal
 * conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroWrite;

/**
 * @brief ioCoroSyscall
 *
 * @note ioCoroCompletedWrite(...) equal to first call ioCoroWrite(...) and than
 * sock.ShutDownWrite()
 *
 * @ingroup user-context
 */
struct ioCoroCompletedWrite;

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroConnect(stream, host);
 * @arg stream, a Stream type
 * @arg host, char const*, the server hostname("x.x.x.x:port" or "domain
 * name:port")
 * @return void
 *
 * @note ioCoro-context guarantees that the Conncetion will be finished under
 * normal conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroConnect;

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroReconnect(stream, host);
 * @arg stream, a Stream type
 * @arg host, char const*, the server hostname("x.x.x.x:port" or "domain
 * name:port")
 * @return void
 *
 * @note ioCoro-context guarantees that the Reconncetion will be finished under
 * normal conditions, otherwise the stream status is changed to reflect an error
 *
 * @ingroup user-context
 */
inline ioCoroConnect
ioCoroReconnect(Socket& inS, char const* inHost);

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      co_await ioCoroSuspendFor(stream, elapse);
 *      or
 *      co_await ioCoroSuspendFor([]{
 *        ...
 *        return true;
 *      }, stream, elapse);
 * 
 * @arg stream, a Stream type
 * @arg elapse, duration<..., ...>.
 * @return bool, return true by default.
 *
 * @note either the user does not provide a predecessor task, if it is provided,
 * it must have a bool return value ioCoro-context guarantees that the
 * ioCoroSuspendFor will be finished under normal conditions.
 *
 * @ingroup user-context
 */
#ifdef NEED_IOCORO_TIMER
template<InvokedAndBoolReturn,
         typename,
         typename>
struct ioCoroSuspendFor;
#endif

/**
 * @brief ioCoroSyscall
 *
 * @cond must be in Coroutine to call the syscall
 * @code
 *      auto [ret, idx] = co_await ioCoroRead(stream, buf, num, delim);
 * @arg stream, a Stream type
 * @arg buf, an address of byte chunk
 * @arg num, the chunk max size
 * @arg delim, char const*, the terminate string
 * @return
 *  @arg ret, how many bytes are received
 *  @arg idx, the position of the delim in the buffer(chunk)
 *
 * @note ioCoro-context guarantees that all data is received OR meets the
 * terminate string(delim) under normal conditions, otherwise the stream
 * status is changed to reflect an error
 *
 * @ingroup user-context
 */
struct ioCoroReadUntil;

} // namespace ioCoro

#include "iocoro_syscall.hpp"