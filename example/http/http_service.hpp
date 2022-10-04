#pragma once

#include "http_errors.hpp"
#include "http_chunk.hpp"

using namespace ioCoro;

namespace http {

struct Http
{

/**
 * @brief the max default response time
 */
static constexpr auto DefualtMaxResponseTimeForServer = 10s;
static constexpr auto DefualtMaxResponseTimeForClient = 15s;

        /**
         * @brief the ioCoro Entry for http client end
         * 
         * @arg sock, the stream thrown by ioCoro-context
         * @arg host, hostname
         * @arg path, resource path
         * @arg id, request Number
         * 
         * @ingroup http-client end
         */
        static IoCoro<void> Active(Stream stream, char const* host, char const* path, uint id);

        /**
         * @brief the ioCoro Entry for http server end
         * 
         * @arg sock, the stream thrown by ioCoro-context
         * 
         * @ingroup http-server end
         */
        static IoCoro<void> Passive(Stream streaming);
};

} // namespace http