#include "answerer.hpp"
#include "swap.hpp"

int main()
{
        Server<Swap<1>> swap1{":4093"};
        Server<Swap<2>> swap2{":4094"};
        Server<Swap<3>> swap3{":4095"};
        Server<Swap<4>> swap4{":4096"};

        Server<Answerer> server{":1024"};

        server.Run();
        swap4.Run(); swap3.Run(); swap2.Run(); swap1.Run();
}