#include "tree.hpp"

int main(int args, char** argv)
{
        char const* host = Hosts[argv[1]].data();

        At at{2};

        printf("The Tree root: %s(%s)\n", argv[1], host);

        at.Submit(host);

        at.JoinRightNow();       
}