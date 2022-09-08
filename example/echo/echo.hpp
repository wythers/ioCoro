#include <iocoro/iocoro.hpp>

#include <iostream>

using namespace ioCoro;

using std::cout;
using std::endl;

struct Echo
{

        static IoCoro<void> Active(Socket sock, char const* ip, int port)
        {
                if (sock)
                {
                        cout << "Sock init failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }

                co_await ioCoroConnect(sock, ip, port);
                if (sock)
                {
                        cout << "ioCoroConnect failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }

                char const* str  = "HELLO, IOCORO!\n";

                co_await ioCoroWrite(sock, str, strlen(str));
                if (sock)
                {
                        cout << "ioCoroWrite failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }
                sock.ShutdownWrite();

                char buf[24]{};

                co_await ioCoroRead(sock, buf, sizeof(buf));
                if (sock)
                {
                        cout << "ioCoroRead failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }

                cout << buf;

                sock.Unhide();
                co_return;

        }

        static IoCoro<void> Passive(Socket sock)
        {
                if (sock)
                {
                        cout << "Sock init failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }

                char buf[24]{};

                co_await ioCoroRead(sock, buf, sizeof(buf));
                if (sock)
                {
                        cout << "ioCoroRead failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }

                for (uint i = 0; i < strlen(buf); ++i)
                {
                        if (buf[i] >= 'A' && buf[i] <= 'Z')
                        {
                                buf[i] += ('a' - 'A');
                        }
                }

                co_await ioCoroWrite(sock, buf, strlen(buf));
                if (sock)
                {
                        cout << "ioCoroWrite failed, " << sock.ErrorMessage() << endl;
                        co_return;
                }
                sock.ShutdownWrite();

                sock.Unhide();
                co_return;
        }
};