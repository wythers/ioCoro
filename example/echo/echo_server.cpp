#include "echo.hpp"

int main()
{
        Server<Echo> server{":1024"};

        server.Run();
}