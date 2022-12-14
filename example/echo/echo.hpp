#pragma once

#define THREADS_NUM 4
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

using namespace ioCoro;

/**
 * @brief the echo client sends the Uppercase string to the echo server, 
 * and then the server translates the string to the Lowercase, and sends it back,  
 * at final the client displays the Lowercases just obtained from the server on the terminal.
 */
struct Echo
{

/**
 * @brief the max default response time
 */
static constexpr auto DefualtMaxResponseTimeForServer = 2s;
static constexpr auto DefualtMaxResponseTimeForClient = 2s;

        struct DataChunk
        {
                char const* uppercase{};
                int id{};
        };

        /**
         *  the ioCoro entry of client end
         */
        static IoCoro Active(Stream stream, char const* host, DataChunk chunk)
        {
                // simpler to use the data chunk, dont worry this expression will be optimized by the compiler
                auto const& [uppercases, id] = chunk;

                // guarantees the stream(socket) reclaimed by the ioCoro-context
                unique_stream cleanup([=]{
                        printf("REQUEST #%d has completed.\n", id);
                }, stream);

                // ensure REQUEST completion within the maximum time frame, and the time frame is 2s
                {
                        DeadLine line([&]{
                                stream.Close();
                        }, stream, DefualtMaxResponseTimeForClient);

                        // try to connect the server
                        co_await ioCoroConnect(stream, host);
                        if (CHECKINGERROR(stream, id))
                                co_return;

                        // try to send the Uppercase string to the server, and then shutdown the Write stream
                        co_await ioCoroCompletedWrite(stream, uppercases, strlen(uppercases));
                        if (CHECKINGERROR(stream, id))
                                co_return;

                        char lowercase[32]{};

                        // try to get the lowercase string sent back from the server, and and then shutdown the Read stream
                        co_await ioCoroCompletedRead(stream, lowercase, sizeof(lowercase));
                        if (CHECKINGERROR(stream, id))
                                co_return;
                        
                        // display the lowercase string on the terminal
                        printf("%s", lowercase);
                } // deadline end 

                co_return;
        }
        
        /**
         *  the ioCoro entry of server end
         */
        static IoCoro Passive(Stream streaming)
        {
                // guarantees the stream(socket) reclaimed by the ioCoro-context
                unique_stream cleanup([]{
                        printf("An ECHO-request just completed\n");
                }, streaming);

                // ensure REQUEST completion within the maximum time frame
                {
                        DeadLine line([&]{
                                streaming.Close();
                        }, streaming, DefualtMaxResponseTimeForServer);

                        char lowercases[32]{};

                        // try to get the Uppercase string sent from the client, and then shutdown the Read stream
                        ssize_t ret = co_await ioCoroCompletedRead(streaming, lowercases, sizeof(lowercases));
                        if (CHECKINGERROR(streaming))
                                co_return;
                        
                        // translate the Uppercase string just obtained from the client to the lowercase
                        to_lowercases(lowercases, lowercases, ret);

                        // try back to send the lowercase string just translated to the client, and then shutdown the Write stream
                        co_await ioCoroCompletedWrite(streaming, lowercases, ret);
                        if (CHECKINGERROR(streaming))
                                co_return;
                } // deadline end

                co_return;
        }


        
        /**
         *  the set of interfaces for error handling
         */ 

        static bool CHECKINGERROR(Stream const& stream)
        {
                return CHECKINGERROR(stream, 0, 0);
        }

        static bool CHECKINGERROR(Stream const& stream, uint id)
        {
                return CHECKINGERROR(stream, id, "ECHO-REQUSET");
        }

        static bool CHECKINGERROR(Stream const& stream, uint id, char const* label)
        {
                if (stream)
                {
                        if (!label)
                        {
                                fprintf(stderr, 
                                "IOCORO: %s, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                                "Oops, the server responded with an error",
                                stream.StateCode(),
                                stream.ErrorMessage().data());
                        } else {
                                fprintf(stderr, 
                                "%s #%d%s, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                                label,
                                id,
                                " failed!",
                                stream.StateCode(),
                                stream.ErrorMessage().data()); 
                        }

                        return true;
                }

                return false;
        }

        static void to_lowercases(char* dest, char const* src, int len)
        {
                for (int i = 0; i < len; ++i)
                {
                        if (src[i] <= 'Z' && src[i] >= 'A')
                        {
                                dest[i] = 'a' + src[i] - 'A';
                        } else {
                                dest[i] = src[i];
                        }
                }
        }

};