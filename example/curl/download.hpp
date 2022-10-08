#pragma once

#include "data_chunk.hpp"

#define THREADS_NUM 2
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>
#include <unistd.h>

using namespace ioCoro;

struct Curl
{

/**
 * @brief the max default response time
 */
static constexpr auto DefaultMaxTimeOfWaiting = 2s;

        static IoCoro Active(Stream stream, char const* url)
        {
                unique_stream cleanup([&]{
                        if (!stream)
                                printf("Download just finished...\n");
                }, stream);

                DataChunk chunk{url};

                string request;

                request += "GET " + chunk.resrc_name + " HTTP/1.1\r\n";
                request += "Host: " + chunk.Host + "\r\n\r\n";
               
                {
                        DeadLine line([&]{
                                stream.Close();
                                printf("the CURL-request has been a timeout\n");
                        }, stream, DefaultMaxTimeOfWaiting);
                
                        co_await ioCoroConnect(stream, chunk.host.data());
                        if (CHECKINGERROR(stream))
                                co_return;

                        co_await ioCoroCompletedWrite(stream, request.data(), request.size());
                        if (CHECKINGERROR(stream))
                                co_return;
                     
                        auto [num, pos] = co_await ioCoroReadUntil(stream, chunk.headers, 4096, "\r\n\r\n");
                        if (CHECKINGERROR(stream) || ::CHECKINGERROR(chunk))
                               co_return;
                        
                        ssize_t ret = co_await ioCoroCompletedRead(stream, chunk.resrc.data(), chunk.resrc.size());
                        if (CHECKINGERROR(stream))
                                co_return;

                        FILE* file = fopen(chunk.file_name.data(), "a");
                        fwrite(chunk.headers + pos + 4, 1, num - pos - 5, file);
                        fwrite(chunk.resrc.data(), 1, ret, file);
                }

                co_return;
        };

        static bool CHECKINGERROR(Stream& stream)
        {
                if (stream)
                {
                        fprintf(stderr, 
                        "CURL-request failed!, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                        stream.StateCode(),
                        stream.ErrorMessage().data());
                        
                        return true;
                }

                return false;
        }

};