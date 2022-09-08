#include <iocoro/iocoro.hpp>

#include <iostream>
#include <string.h>

#include <sys/stat.h>
#include <sys/mman.h>

using namespace ioCoro;

using std::cout;
using std::endl;

struct Echo
{

        static IoCoro<void> Active(Socket sock, char const* ip, int port)
        {
                co_await ioCoroConnect(sock, ip, port);
                if (sock)
                {
                        cout << "connect failed\n";
                        co_return;
                }

                int ret = 0;
                struct stat st{};

                ret = stat("/home/yang/linux.pdf", &st);
                if (ret == -1)
                {
                        cout << "stat failed\n";
                        co_return;
                }

                cout << st.st_size << endl;

                int fd = open("/home/yang/linux.pdf", O_RDONLY);
                if (fd == -1)
                {
                        cout << "open failed\n";
                        co_return;
                }

                void* p = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
                if (p == MAP_FAILED)
                {
                        cout << "mmap failed\n";
                        co_return;
                }

                cout << "mmap ok" << endl;

                ssize_t back = co_await ioCoroWrite(sock, p, st.st_size);

                cout << back << endl;

                sock.ShutdownWrite();

                munmap(p, st.st_size);

                char buf[24]{};

                co_await ioCoroRead(sock, buf, 24);

                cout << "back code: " << buf;

                sock.Unhide();

                co_return;
        }

        static IoCoro<void> Passive(Socket sock)
        {

                int fd = open("tmp.pdf", O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
                if (fd == -1)
                {
                        cout << "open failed\n";
                        co_return;
                }

                void* p = mmap(0, 12046000, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (p == MAP_FAILED)
                {
                        cout << "mmap failed\n";
                        perror("mmap");
                        co_return;
                }

    //            char buf[1025]{};

                ssize_t back = co_await ioCoroRead(sock, p, 12046000);

                int ret = ::write(fd, p, back);
               // int ret = ::read(fd, "fucking shut\n", 13);

                if (ret == -1)
                {
                        cout << "::read() failed\n";
                } else 
                cout << ret << endl;

          //      ssize_t back = ::recv(sock.GetFd(), p, 12046000, MSG_WAITALL);

                cout << back << endl;

                munmap(p, 12046000);

                back = co_await ioCoroWrite(sock, "ok\n", 3);

                cout << back << endl;
                sock.ShutdownWrite();

                sock.Unhide();

                co_return;
        }
};