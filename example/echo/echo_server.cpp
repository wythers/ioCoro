#include "echo.hpp"

int main()
{
        Server<Echo> server{nullptr, 1024};

        server.Run();
}