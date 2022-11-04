#pragma once

#include "payload.pb.h"

#define THREADS_NUM 4
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

#include <unordered_map>

using namespace ioCoro;

using std::mutex;
using std::lock_guard;
using std::string;
using std::vector;
using std::unordered_map;

struct VertexRadicalInfo {

        mutex mtx{};
        string parent{};
        uint expected_msg{};
        google::protobuf::RepeatedPtrField<adi::PayLoad_Pair> store{};

        string id{};
        string city{};
        vector<string> neighbors{};
};

struct VertexInfo : VertexRadicalInfo {
        int temperature{};
};

static inline unordered_map<string_view, string_view> Cities =
{
        { "localhost:1024", "GuangZhou" },
        { "localhost:1025", "ZhenZhou" },
        { "localhost:1026", "BeiJing" },
        { "localhost:1027", "ShenZhen" }, 
        { "localhost:1028", "XinJiang" },  
        { "localhost:1029", "WuHan" }       
};


struct S 
{
        static IoCoro Active(Stream stream, string host, string bytes)
        {
                unique_stream cleanup(stream);

                stream.KeepAlive();

                {
                        DeadLine L1([&]{
                                stream.Close();
                                printf("The remote neighbor[#%s] made some errors", host.data());
                        }, stream, 1s);

                        co_await ioCoroConnect(stream, host.data());
                }

                for (;;)
                {

                        {
                                DeadLine L2([&]{
                                        stream.Close();
                                        printf("The Sender got a timeout");
                                }, stream, 1s);

                                co_await ioCoroCompletedWrite(stream, bytes.data(), bytes.size());
                        }

                        if (stream)
                        {
                                if (stream.StateCode() == errors::timed_out)
                                {
                                        DeadLine L3([&]{
                                                stream.Close();
                                                printf("The remote neighbor[#%s] made some errors", host.data());
                                        }, stream, 1s);
                                        co_await ioCoroReconnect(stream, host.data());
                                } 

                                if (!stream)
                                        continue;
                        }

                        co_return;
                }

                co_return;
        }
};

using Sender = Client<S>;

