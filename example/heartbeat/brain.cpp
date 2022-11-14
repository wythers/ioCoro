#include "brain.hpp"

int main()
{
        Server<Brain> server(":1024");
        Fctx Op{};

        server.Run(&Op);
}