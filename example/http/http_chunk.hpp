#pragma once

#include "http_errors.hpp"

namespace http {

#define MAX_STATUS_CODE 8
#define MAX_STATUS_MESSAGE 32
#define MAX_PATH_SIZE 64
#define MAX_VERSION_SIZE 32

#define MAX_HEADER_SIZE 256

        struct http_helper
        {
                template<typename H, typename... Args>
                static constexpr void CreateHeaderLine(string& hdr, H&& h, Args&&... args)
                {
                        constexpr size_t n = sizeof...(Args);

                        hdr += std::forward<H>(h);
                        if constexpr (n)
                                CreateHeaderLine(hdr, std::forward<Args>(args)...);
                        else 
                                hdr += "\r\n";
                }
        };

        struct http_response
        {
                char m_status_code[MAX_STATUS_CODE]{};
                char m_status_message[MAX_STATUS_MESSAGE]{};
                char m_http_version[MAX_VERSION_SIZE]{};

                unordered_map<string_view, string_view> m_headers{};
                char m_response_body[MAX_HEADER_SIZE]{};
                vector<char> m_entity_body{};

                std::pair<char const*, char const*> const parse_status_line();

                std::pair<char const*, char const*> const parse_header_line();

                bool had_header_line();

        };

        struct http_request
        {
                char m_method[MAX_STATUS_CODE]{};
                char m_src_path[MAX_PATH_SIZE]{};
                char m_http_version[MAX_VERSION_SIZE]{};

                char m_request_body[MAX_HEADER_SIZE]{};
                vector<char> m_entity_body{};

                int parse_status_line();

                int respond_to_request();
        };

} // namespace http