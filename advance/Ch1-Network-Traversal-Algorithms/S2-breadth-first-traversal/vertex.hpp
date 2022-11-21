#pragma once

#include "../../common.hpp"
#include "vertex_info.hpp"

class Vertex
{
        enum {
                START = 0,
                GO,
                BACK
        };

        struct A
        {
                static IoCoro Passive(Stream streaming, Vertex* vx)
                {
                        unique_stream cleanup(streaming);

                        char buf[256]{};
                        ssize_t ret{};

                        {
                                DeadLine L1([&]{
                                        streaming.Close();
                                }, streaming, 1s);              
                                ret = co_await ioCoroCompletedRead(streaming, buf, 255);
                        }
                        if (streaming)
                                co_return;

                        adi::Payload load{};
                        load.ParseFromArray(buf, ret);

                        Anchor(load)
                        .Condition<START>([=](adi::Payload& res){
                                vx->handle_START(res);
                        })
                        .Condition<GO>([=](adi::Payload& res){
                                vx->handle_GO(res);
                        })
                        .Condition<BACK>([=](adi::Payload& res){
                                vx->handle_BACK(res);
                        });

                        co_return;
                }
        };

        using Acceptor = Server<A>;

public:
        Vertex(char const* host)
        : id(host)
        , m_acpor(host, 4)
        {}

        void Launch()
        {
                printf("#%s[%s] is launched right now...\n", Cities[id].data(), id.data());
                m_acpor.Run(this);
        }

        void handle_START(adi::Payload& res)
        {
                res.set_type(GO);
                res.set_dist(-1);

                // line 1
                m_sdr.Submit(id, res.SerializeAsString());
        }

        void handle_GO(adi::Payload& res)
        {
                bool flag = false;
                {
                        lock_guard<mutex> locked(mtx);
                        
                        // line 2
                        if (parent == "")
                        {
                                if (res.dist() != -1)
                                {
                                        // line 3
                                        parent = res.from();
                                        // line 3
                                        expected_msg = neighbors.size() - 1;
                                } else {
                                        expected_msg = neighbors.size();
                                }
                                // line 3
                                level = res.dist() + 1;

                                flag =  true;
                               // line 9
                        } else if (level > res.dist() + 1) {
                                // line 10
                                parent = res.from();
                                children.clear();
                                level = res.dist() + 1;
                                expected_msg = neighbors.size() - 1;

                                flag = true;
                        }
                }

                if (flag)
                {
                        // line 5 and line 13  
                        if ((neighbors.size() - 1) == 0 && res.dist() != -1)
                        {
                                res.set_type(BACK);
                                int tmp = res.dist();
                                res.set_dist(tmp + 1);

                                string from = res.from();
                                res.set_from(id);

                                res.set_resp(adi::Payload::yes);

                                // line 13
                                m_sdr.Submit(from, res.SerializeAsString());
                                
                                return;
                        } else {
                                res.set_type(GO);
                                res.set_dist(res.dist() + 1);
                                string from = res.from();
                                res.set_from(id);

                                string tmp = res.SerializeAsString();

                                // line 14
                                for (auto& str : neighbors)
                                {
                                        if (str != from)
                                                m_sdr.Submit(str, tmp);
                                }

                                return;
                        }
                        
                }

                res.set_type(BACK);
                res.set_resp(adi::Payload::no);
                res.set_dist(res.dist() + 1);
                string from = res.from();
                res.set_from(id);

                // line 16
                m_sdr.Submit(from, res.SerializeAsString());                   
        }

        void handle_BACK(adi::Payload& res)
        {
                int tmp = 0;
                bool flag = false;
                {
                        lock_guard<mutex> locked(mtx);

                        // line 19
                        if ((level + 1) == res.dist())
                        {
                                flag = true;
                                // line 20
                                if (res.resp() == adi::Payload::yes)
                                        children.push_back(res.from());
                                // line 21
                                tmp = --expected_msg;
                        }
                }

                if (flag && tmp == 0)
                {
                        // theoretically parent becomes const, and no mtx is needed.
                        if (parent != "")
                        {
                                res.set_dist(level);
                                res.set_from(id);
                                res.set_type(BACK);
                                res.set_resp(adi::Payload::yes);

                                // line 23
                                m_sdr.Submit(parent, res.SerializeAsString());
                        } 

                        // aggregate children to the str
                        string out{};
                        for (auto& str : children)
                        {
                                out += Cities[str];
                                out += "(";
                                out += str; 
                                out += ") ";
                        }       

                        // print all children
                        if (!out.empty())
                                printf("The Vertex[%s:%s]'s children: %s\n",
                                        Cities[id].data(),      
                                        id.data(),
                                        out.data());
                }
        }

        void set_neighbor(char const* neighbor)
        {
                neighbors.emplace_back(neighbor);
        }

private:
        mutex mtx{};

        string parent{};
        uint expected_msg{};
        int level{};
        vector<string> children{};

        vector<string> neighbors{};

        string id{};

        Sender m_sdr{4};

        Acceptor m_acpor{":1024", 4};
};