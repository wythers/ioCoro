/**
 *         
 */
#include "echo.hpp"

/**
 * @brief five Workers in ioCoro-context under the load of 5000 requests/second are working well on my computer,
 * why do not try to run it on your computer?
 */
int main()
{
        Client<Echo> client{};

        for (int i = 0; i < 100000;)
        {
                for (int j = 0; j < 5000; ++j)
                {
                        client.Submit("localhost:1024", ++i);
                }

                sleep(1);
        }

        client.JoinRightNow();
}



