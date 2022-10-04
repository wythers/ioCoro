#include "http_service.hpp"

using namespace http;

int main()
{
        Server<Http> server{":1024"};

        server.Reserver(1200);

        server.Run();
}