#pragma once

#define THREADS_NUM 4
#include <iocoro/iocoro.hpp>

#include <unordered_map>

using namespace ioCoro;
using std::unordered_map;

static inline unordered_map<int, char const*> host_num = {
        {1, ":4094"},
        {2, ":4095"},
        {3, ":4096"},
        {4, ":1024"}
};

template<size_t Idx>
struct Swap
{
        static IoCoro Passive(Stream streaming, Stream stream)
        {
                unique_stream cleanup(streaming, stream);

                char buf[32]{};

                ssize_t ret = co_await ioCoroCompletedRead(streaming, buf, sizeof(buf));
                if (streaming)
                        co_return;
                
                co_await ioCoroConnect(stream, host_num[Idx]);
                if (stream)
                        co_return;
                
                co_await ioCoroCompletedWrite(stream, buf, ret);
                if (stream)
                        co_return;
                
                ret = co_await ioCoroCompletedRead(stream, buf, sizeof(buf));
                if (stream)
                        co_return;
                
                co_await ioCoroCompletedWrite(streaming, buf, ret);

                co_return;
        }
};