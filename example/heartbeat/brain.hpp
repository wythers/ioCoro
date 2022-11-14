#pragma once

#include "random.hpp"

#define THREADS_NUM 2
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

#include <mutex>

using std::mutex;
using namespace ioCoro;
using namespace std::string_view_literals;

struct Fctx
{
        mutex mtx{};
        std::string status{};

        int const steps{10};
};

struct Brain
{

        static IoCoro Passive(Stream streaming, Fctx* Op)
        {
                // try to get control of the foot
                Op->mtx.lock();
                
                unique_stream cleanup(streaming);

                co_await ioCoroWrite(streaming, "s\n", 2);


                for (;;)
                {
                        char buf[8]{};

                        // the brain waits for 1 second at most.
                        // if the feet do not respond, it temporarily thinks that the feet are out of control.
                        {
                                DeadLine L1([&]{
                                        streaming.Close();
                                }, streaming, 1s);
                                co_await ioCoroReadUntil(streaming, buf, sizeof(buf), "\n");
                        }

                        if (streaming)
                        {
                                if (Op->status.size() != 0)
                                        Op->status.pop_back();
                                Op->mtx.unlock();
                                co_return;
                        }
                        
                        if ("ok\n"sv == buf)
                        {
                
                                if (Op->status.size() >= Op->steps)
                                {
                                        
                                        co_await ioCoroWrite(streaming, "f\n", 2);
                                        Op->mtx.unlock();
                                        co_return;
                                }

                                Op->status += "s";
                                printf("\r__________");
                                printf("\r%s", Op->status.data());
                                fflush(stdout);
                        }
                

                        co_await ioCoroSuspendFor(streaming, 1s);

                        co_await ioCoroWrite(streaming, "s\n", 2);
                }
        }
};