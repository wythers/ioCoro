#include "test.hpp"

int main()
{
        Client<test> client;

        client.Submit("127.0.0.1", 1024);

        client.Run();       

        client.join(); 
}