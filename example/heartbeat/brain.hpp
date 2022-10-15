#pragma once

#include "random.hpp"

#define THREADS_NUM 2
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

#include <mutex>

using std::mutex;
using namespace ioCoro;
using namespace std::string_view_literals;

struct Brain
{

static inline mutex global_mtx{};
static inline std::string status{};

static inline int const steps{10};

        static IoCoro Passive(Stream streaming)
        {
                // waitting for control of the feet
                global_mtx.lock();

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
                                if (status.size() != 0)
                                        status.pop_back();
                                global_mtx.unlock();
                                co_return;
                        }
                        


                        if ("ok\n"sv == buf)
                        {
                
                                if (status.size() >= steps)
                                {
                                        
                                        co_await ioCoroWrite(streaming, "f\n", 2);
                                        global_mtx.unlock();
                                        co_return;
                                }

                                status += "s";
                                printf("\r__________");
                                printf("\r%s", status.data());
                                fflush(stdout);
                        }
                

                        co_await ioCoroSuspendFor(streaming, 1s);

                        co_await ioCoroWrite(streaming, "s\n", 2);
                }
        }
};