#pragma once

#include <atomic>

using std::atomic;

namespace ioCoro {

namespace mm {

inline constexpr auto rx = std::memory_order::relaxed;
inline constexpr auto rel = std::memory_order::release;
inline constexpr auto acq = std::memory_order::acq_rel;


template<typename T>
inline constexpr auto rx_load(atomic<T>& x)
{
        return x.load(rx);
}

template<typename T>
inline constexpr auto rx_store(atomic<T>& x, T n)
{
        return x.store(n, rx);
}

template<typename T>
inline constexpr auto acq_load(atomic<T>& x)
{
        return x.load(acq);
}

template<typename T>
inline constexpr auto rel_store(atomic<T>& x, T n)
{
        return x.store(n, rel);
}

template<typename T>
inline constexpr auto rx_compare_exchange_weak(atomic<T>& x, T& n, T m)
{
        return x.compare_exchange_weak(n, m, rx, rx);
}

template<typename T>
inline constexpr auto rx_compare_exchange_strong(atomic<T>& x, T& n, T m)
{
        return x.compare_exchange_strong(n, m, rx, rx);
}

template<typename T>
inline constexpr auto rel_compare_exchange_weak(atomic<T>& x, T& n, T m)
{
        return x.compare_exchange_weak(n, m, rel, rx);
}

template<typename T>
inline constexpr auto rel_compare_exchange_strong(atomic<T>& x, T& n, T m)
{
        return x.compare_exchange_strong(n, m, rel, rx);
}

template<typename T>
inline constexpr auto acq_compare_exchange_weak(atomic<T>& x, T& n, T m)
{
        return x.compare_exchange_weak(n, m, acq, rx);
}

template<typename T>
inline constexpr auto acq_compare_exchange_strong(atomic<T>& x, T& n, T m)
{
        return x.compare_exchange_strong(n, m, acq, rx);
}

} // namespace mm

using namespace mm;

} // namespace ioCoro