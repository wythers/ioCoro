#pragma once

#define THREADS_NUM 4
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

#include <unordered_map>
#include <string_view>

using namespace ioCoro;
using std::unordered_map;

static inline unordered_map<string_view, const char*> prices = {
        {"jeep", "$20.000"},
        {"SUV", "$30.000"},
        {"van", "$40.000"},
        {"vehicle", "$50.000"}      
};

struct Answerer
{
static constexpr auto DefaultMaxTimeOfWaiting = 2s;

        static IoCoro Passive(Stream streaming)
        {
                unique_stream cleanup([&]{
                        if (!streaming)
                                printf("A response just completed.\n");
                        else 
                                printf("Oop, a response got an error.\n");
                }, streaming);
                
                char buf[32]{};

                {
                        DeadLine line([&]{
                                streaming.Close();
                        }, streaming, DefaultMaxTimeOfWaiting);

                        co_await ioCoroCompletedRead(streaming, buf, sizeof(buf));
                        if (streaming)
                                co_return;
                        
                        if (!prices.count(buf))
                                co_await ioCoroCompletedWrite(streaming, "unknown", 7);
                        else 
                                co_await ioCoroCompletedWrite(streaming, prices[buf], strlen(prices[buf]));
                }

                co_return;
        }
};