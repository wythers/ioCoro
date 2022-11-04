#include "at.hpp"

int main(int args, char** argv)
{
        char const* host = Hosts[argv[1]].data();

        At at{};

        at.Submit(host, "temperature");

        at.JoinRightNow();
}