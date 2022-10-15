#include "feet.hpp"

int main()
{
        Client<Feet> client{};       

        client.Submit(":1024");

        client.JoinRightNow();
}