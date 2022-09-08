/**
 *         
 */
#include "echo.hpp"

int main()
{
        Client<Echo> client{};

        client.Submit("127.0.0.1", 1024);

        client.Run();       

        client.join();    
}



