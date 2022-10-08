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
                // guarantees the stream(socket) reclaimed by the ioCoro-context
                unique_stream cleanup([&]{
                        if (!stream)
                                printf("Download just finished...\n");
                }, stream);

                // initializes data chunk
                DataChunk chunk{url};

                // generates http request headers
                string request;

                request += "GET " + chunk.resrc_name + " HTTP/1.1\r\n";
                request += "Host: " + chunk.Host + "\r\n\r\n";
               
                // ensure REQUEST completion within the maximum time frame, and the time frame is 2s
                {
                        DeadLine line([&]{
                                stream.Close();
                                printf("the CURL-request has been a timeout\n");
                        }, stream, DefaultMaxTimeOfWaiting);
                
                        // try to connect the server
                        co_await ioCoroConnect(stream, chunk.host.data());
                        if (CHECKINGERROR(stream))
                                co_return;

                        // try to send the Request Headers to the server, and then shutdown the Write Stream
                        co_await ioCoroCompletedWrite(stream, request.data(), request.size());
                        if (CHECKINGERROR(stream))
                                co_return;

                        // try to get the Reponse Headers
                        auto [num, pos] = co_await ioCoroReadUntil(stream, chunk.headers, 4096, "\r\n\r\n");
                        if (CHECKINGERROR(stream) || ::CHECKINGERROR(chunk))
                               co_return;
                        
                        // try to get the reuqest entity body and shutdown the Read Stream
                        ssize_t ret = co_await ioCoroCompletedRead(stream, chunk.resrc.data(), chunk.resrc.size());
                        if (CHECKINGERROR(stream))
                                co_return;

                        // save the requested resources to the current directory
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