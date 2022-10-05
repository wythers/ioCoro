#include "echo.hpp"

int main()
{
        Server<Echo> server{":1024"};

        server.Reserver(10000);

        server.Run();
}