#include "http_service.hpp"

using namespace http;

IoCoro Http::Active(Stream stream, char const* host, char const* path, uint id)
{
        // guarantees the stream(socket) reclaimed by the ioCoro-context
        unique_stream cleanup([=]{
                printf("REQUEST #%d%s" , id, " has completed.\n");
        }, stream); 

        http_response res{};
        string GET_request{};
        ssize_t ret{};

        // ensure REQUEST completion within the maximum time frame
        {
                DeadLine line([&]{
                        stream.Close();
                }, stream, DefualtMaxResponseTimeForClient);
        
                // try to connect the server
                co_await ioCoroConnect(stream, host);
                if (stream)
                {
                        CHECKINGERROR(stream, id);
                        co_return;
                }

                // Create the Headers
                http_helper::CreateHeaderLine(GET_request, 
                                "GET ", path, " HTTP/1.1\r\n"
                                "Host: localhost\r\n"
                                "Connection: close\r\n");

                // try to send the Request and shutdown the Write stream
                co_await ioCoroCompletedWrite(stream, GET_request.data(), GET_request.size());
                if (stream)
                {
                        CHECKINGERROR(stream, id);
                        co_return; 
                }

                
                // try to get the Reponse status line
                auto [n, p] = co_await ioCoroReadUntil(stream, res.m_response_body, MAX_HEADER_SIZE - 1, "\r\n");
                if (stream)
                {
                        CHECKINGERROR(stream, id);
                        co_return;
                }

                // parse the status line
                if (CHECKINGERROR(res.parse_status_line(), id))
                        co_return;
                
                // determine whether the Headers have been all in buf
                if (!res.had_header_line())
                {
                        // try to get the all Headers
                        co_await ioCoroReadUntil(stream, res.m_response_body + n, MAX_HEADER_SIZE - n - 1, "\r\n\r\n");
                        if (stream)
                        {
                                CHECKINGERROR(stream, id);
                                co_return;
                        }
                }
                
                // processing and analyzing headers
                if (CHECKINGERROR(res.parse_header_line(), id))
                        co_return;
                

                // try to get the reuqest entity body and shutdown the Read Stream
                ret = co_await ioCoroCompletedRead(stream, res.m_entity_body.data(), res.m_entity_body.size());
                if (stream)
                {
                        CHECKINGERROR(stream, id);
                        co_return;
                }

        } // deadlien end
                
        // display the File and Header to the terminal
        ::write(1, res.m_response_body, strlen(res.m_response_body));
        ::write(1, res.m_entity_body.data(), ret);

        co_return;
}

IoCoro Http::Passive(Stream streaming)
{
        // guarantees the stream(socket) reclaimed by the ioCoro-context
        unique_stream cleanup([]{
                printf("A GET-request just completed\n");
        }, streaming);

        http_request res{};
        string status_line{};

        // ensure REQUEST completion within the maximum time frame
        {
                DeadLine line([&]{
                        streaming.Close();
                }, streaming, DefualtMaxResponseTimeForServer);

                // try to get the request statu line
                co_await ioCoroReadUntil(streaming, res.m_request_body, MAX_HEADER_SIZE - 1, "\r\n");
                if (streaming)
                {
                        CHECKINGERROR(streaming);
                        co_return;
                }

                streaming.ShutdownRead();

                // parse the request status line
                int status_code = res.parse_status_line();

                // send the error code to the peer if the request status line is abnormal
                if (status_code != 0)
                {
                        http_helper::CreateHeaderLine(status_line,
                                                        "HTTP/1.1 ", http_status_table[status_code], "\r\n");

                        co_await ioCoroCompletedWrite(streaming, status_line.data(), status_line.size());
                        if (streaming)
                        {
                                CHECKINGERROR(streaming);
                                co_return;
                        }

                        co_return;
                }

                // prepare the requested resource
                status_code = res.respond_to_request();
                
                // send the error code to the peer if the resource is abnormal
                if (status_code != 0)
                {
                        http_helper::CreateHeaderLine(status_line,
                                                        "HTTP/1.1 ", http_status_table[status_code], "\r\n");

                        co_await ioCoroCompletedWrite(streaming, status_line.data(), status_line.size());
                        if (streaming)
                        {
                                CHECKINGERROR(streaming);
                                co_return;
                        }

                        co_return;
                }

                // create ordinary Response status line and Headers
                http_helper::CreateHeaderLine(status_line,
                                                "HTTP/1.1 ", http_status_table[200], "\r\n",
                                                "Content-Length: ", std::to_string(res.m_entity_body.size()), "\r\n");

                // try to send the Status, the Headers and the Resource to the peer
                co_await ioCoroWrite(streaming, status_line.data(), status_line.size());
                co_await ioCoroCompletedWrite(streaming, res.m_entity_body.data(), res.m_entity_body.size());

       } // dealine end

        co_return;
}