#include <iocoro/iocoro.hpp>

#include <iocoro/guard.hpp>
#include <iostream>
#include <string>
#include <utility>

#include <sys/mman.h>

using namespace ioCoro;
using namespace std;

struct test
{
        static IoCoro<void> Passive(Socket sock)
        {
                unique_socket cleanup(sock);

                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }
                
                void *p = mmap(0, 22045800, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

                auto ret = co_await ioCoroReadUntil(sock, p, 22045800, "\n\t\n\t");
                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }

                cout << "size: " << ret.first << " pos: " << ret.second << endl;

                size_t num = co_await ioCoroRead(sock, p, 22045800);
                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }

                cout << "remaining size: " << num << endl;

                cout << "total size: " << num + ret.first << endl;
                
                co_await ioCoroWrite(sock, "ok\n\t\n\t", 6);
                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }

                co_return;
        }

        static IoCoro<void> Active(Socket sock, char const* ip, int port)
        {
                unique_socket cleanup(sock);

                co_await ioCoroConnect(sock, ip, port);
                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }

                void *p = mmap(0, 22045800, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

                strncpy(static_cast<char*>(p) + 12045800, "\n\t\n\t", 5);



                size_t num = co_await ioCoroWrite(sock, p, 22045800);
                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }

                cout << "write num: " << num << endl;

                sock.ShutdownWrite();

                char buf[32];
                auto ret = co_await ioCoroReadUntil(sock, buf, 32, "\n\t\n\t");
                if (sock)
                {
                        cout << sock.ErrorMessage() << endl;
                        co_return;
                }

                cout << "size: " << ret.first << " pos: " << ret.second << endl;
                buf[ret.second] = '\0';
                cout << buf << endl;

                co_return;
        }

};