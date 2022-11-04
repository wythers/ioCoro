#include "vertex.hpp"
#include "random.hpp"

int main(int args, char** argv)
{
        Vertex v{argv[1]};

        v.set_id(argv[1]);
        v.set_city(Cities[argv[1]].data());
        v.set_temperature(RandInt(-10, 40));

        for (int i = 2; i < args; ++i)
        {
                v.add_neighbor(argv[i]);
        }

        v.Launch();
}