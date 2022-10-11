#pragma once

#define THREADS_NUM 4
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

using namespace ioCoro;


struct Asker
{
static constexpr auto DefaultMaxTimeOfWaiting = 2s;

        static IoCoro Active(Stream stream, char const* host, char const* car)
        {
                char price[16]{};
                unique_stream cleanup([&]{
                        if (!stream)
                                printf("%s: %s\n", car, price);
                        else 
                                printf("Failed to get the price, ERORR:%s\n",
                                stream.ErrorMessage().data());
                }, stream);

                {
                        DeadLine line([&]{
                                stream.Close();
                        }, stream, DefaultMaxTimeOfWaiting);

                        co_await ioCoroConnect(stream, host);
                        if (stream)
                                co_return;
                        
                        co_await ioCoroCompletedWrite(stream, car, strlen(car));
                        if (stream)
                                co_return;
                        
                        co_await ioCoroCompletedRead(stream, price, sizeof(price));
                }

                co_return;
        }
};