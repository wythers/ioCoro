#include "asker.hpp"

int main(int args, char** argv)
{
        Client<Asker> client{};

        client.Submit("localhost:4093", argv[1]);

        client.JoinRightNow();
}