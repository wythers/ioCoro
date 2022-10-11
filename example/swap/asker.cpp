#include "asker.hpp"


/**
 * 
 */
int main(int args, char** argv)
{
        Client<Asker> client{};

        for (int i = 0; i < 10000;)
        {
                for (int j = 0; j < 2000; ++j)
                {
                        client.Submit("localhost:4093", argv[1]);
                        ++i;
                }
                
                sleep(1);
        }

        client.JoinRightNow();
}