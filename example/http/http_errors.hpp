#pragma once

#include <vector>
#include <string>
#include <system_error>
#include <string_view>
#include <unordered_map>
#include <stdio.h>
#include <sys/stat.h>

#define THREADS_NUM 4
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

using namespace ioCoro;
using std::string;
using std::vector;
using std::unordered_map;
using std::string_view;
using namespace std::string_view_literals;

namespace http {

        inline static unordered_map<int, string> http_status_table = 
        {
                { 200, "200 OK" },
                { 404, "404 Not Found" },
                { 413, "413 Request Entity Too Large" },
                { 500, "500 Server Error" },
                { 501, "501 Not Implemented" },
                { 505, "505 HTTP Version Not Supported" }
        };

        inline void CHECKINGERROR(Socket& sock, uint id)
        {
                if (sock.StateCode() == errors::socket_closed) {
                        fprintf(stderr,
                        "REQUEST #%d%s\n",
                        id,
                        "  has been cancelled by the user."
                        );
                }
                else {
                        fprintf(stderr, 
                        "REQUEST #%d%s, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                        id,
                        " failed!",
                        sock.StateCode(),
                        sock.ErrorMessage().data());
                }
        }

        inline void CHECKINGERROR(Socket& sock)
        {
                fprintf(stderr, 
                "IOCORO: %s, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                "Oops, the server responded with an error",
                sock.StateCode(),
                sock.ErrorMessage().data());
        }

        inline bool CHECKINGERROR(std::pair<char const*, char const*> const& status, uint id)
        {
                auto const& [code, message] = status;

                if (code && message)
                {
                        fprintf(stderr, 
                        "REQUEST #%d%s, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                        id,
                        " failed!",
                        atoi(code),
                        message);   
                        
                        return true;
                }

                return false;
        }

        using status_t = std::error_code;

} // namespace http