#include "vertex.hpp"

int main(int args, char** argv)
{

        sigset_t set;
        sigfillset(&set);
        sigdelset(&set, SIGINT);

        sigprocmask(SIG_BLOCK, &set, 0);

        Vertex v{argv[1]};

        for (int i = 2; i < args; ++i)
        {
                v.add_neighbor(argv[i]);
        }

        v.Launch();
}