/**
 *         
 */
#include "echo.hpp"

/**
 * @brief four Workers in ioCoro-context under the load of 30000 requests/second, as a small case, are working well on my computer.
 * why do not try to run it on your computer?
 */
int main()
{
        Client<Echo> client{};

        for (int i = 0; i < 120000;)
        {
                for (int j = 0; j < 30000; ++j)
                {
                        client.Submit("localhost:1024", Echo::DataChunk{ "HELLO, IOCORO!\n", ++i });
                }

                sleep(1);
        }

        client.JoinRightNow();
}



