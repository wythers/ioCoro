#pragma once

#include "random.hpp"

#define THREADS_NUM 2
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

using namespace ioCoro;
using namespace std::string_view_literals;

struct Feet
{
        static IoCoro Active(Stream stream, char const* host)
        {
                unique_stream cleanup([]{
                        printf("\nGod, I finally finished it.\n");
                }, stream);

                {
                        DeadLine L1([&]{
                                stream.Close();
                        }, stream, 2s);
                        co_await ioCoroConnect(stream, host);
                }
                if (stream)
                {
                        printf("Oops, My brain dont work. Help, help ~~~\n");
                        co_return;
                }
                
                // Open the errors::timed_out, let the ioCoro reflect it.
                stream.KeepAlive();
                for (;;)
                {
                        char buf[16]{};

                        co_await ioCoroReadUntil(stream, buf, sizeof(buf), "\n");

                       
                        //the feet may not respond in one of three chance
                        int timeval = RandInt(0, 2);
                        if (!timeval)
                                co_await ioCoroSuspendFor(stream, 1s);  
                        
                        if ("s\n"sv == buf)
                                co_await ioCoroWrite(stream, "ok\n", 3);

                        if ("f\n"sv == buf)
                                co_return;

                        if (stream)
                        {

                                fprintf(stderr, "%d :  %s\n", stream.StateCode(), stream.ErrorMessage().data());
                                if (stream.StateCode() == errors::timed_out)
                                
                                // try to reconnect the brain. 
                                // if it is not completed within 2s, the brain will be judged dead.
                                {
                                        DeadLine L2([&]{
                                                stream.Close();
                                                printf("Ah, my brain is dead.\n");
                                        }, stream, 2s);

                                        fprintf(stderr, "reconnect\n");     
                                        co_await ioCoroReconnect(stream, host);
                                } else 
                                        co_return;       
                        }
                        
                }
                
        }
};