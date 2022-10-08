#include "download.hpp"

int main(int args, char** argv)
{
        Client<Curl> client{};
        
        client.Submit(argv[1]);

        client.JoinRightNow();
}