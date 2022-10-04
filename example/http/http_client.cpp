#include "http_service.hpp"

using namespace http;

int main()
{
        Client<Http> client{};

        client.Submit("localhost:1024", "dummy.html", 1); 

        client.JoinRightNow();
        printf("HTTP CLIENT SAY: all GET requests finished.\n");

        return 0;
}