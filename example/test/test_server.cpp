#include "test.hpp"

int main()
{
        Server<test> Server(nullptr, 1024);

        Server.Run();
}