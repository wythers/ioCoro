#pragma once

#include "payload.pb.h"

#define THREADS_NUM 2
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

#include <unordered_map>

using namespace ioCoro;
using std::unordered_map;

struct Temp
{
        static IoCoro Active(Stream stream, char const* host, char const* request)
        {
                unique_stream cleanup(stream);

                co_await ioCoroConnect(stream, host);
                
                adi::PayLoad load{};

                load.set_type(adi::PayLoad::START);
                load.set_request(request);

                std::string bytes(load.SerializeAsString());

                co_await ioCoroCompletedWrite(stream, bytes.data(), bytes.size());
                

                if (stream)
                {
                        printf("error, %d, %s\n", stream.StateCode(), stream.ErrorMessage().data());
                }

                co_return;
        }
};
using At = Client<Temp>;

static inline unordered_map<string_view, string_view> Hosts =
{
        { "GuangZhou", "localhost:1024" },
        { "ZhenZhou", "localhost:1025" },
        { "BeiJing", "localhost:1026" },
        { "ShenZhen", "localhost:1027" }, 
        { "XinJiang", "localhost:1028" },  
        { "WuHan", "localhost:1029" }    
};