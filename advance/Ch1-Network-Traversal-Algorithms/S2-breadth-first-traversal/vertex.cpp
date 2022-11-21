#include "vertex.hpp"

int main(int args, char** argv)
{
        Vertex v{argv[1]};

        for (int i = 2; i < args; ++i)
        {
                v.set_neighbor(argv[i]);
        }
        
        v.Launch();
}