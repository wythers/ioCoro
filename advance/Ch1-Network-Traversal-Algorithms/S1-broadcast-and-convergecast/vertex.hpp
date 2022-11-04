#pragma once

#include "vertex_info.hpp"

class Vertex
{
        struct A
        {
                static IoCoro Passive(Stream streaming)
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

                        adi::PayLoad load{};
                        load.ParseFromArray(buf, ret);

                        if (load.type() == adi::PayLoad::START)
                        {
                                {
                                        lock_guard<mutex> locked(Vertex::g_vinfo.mtx);

                                        Vertex::g_vinfo.parent = "root";
                                        Vertex::g_vinfo.expected_msg = Vertex::g_vinfo.neighbors.size();
                                }

                                load.set_from(Vertex::g_vinfo.id);
                                load.set_type(adi::PayLoad::GO);
                                load.set_request("temperature");

                                string bytes(load.SerializeAsString());

                                for (uint i = 0; i < Vertex::g_vinfo.neighbors.size(); ++i)
                                {
                                        Vertex::g_sdr.Submit(Vertex::g_vinfo.neighbors[i], bytes);
                                }

                                co_return;
                        }

                        if (load.type() == adi::PayLoad::GO)
                        {
                                string tmp{};
                                string from = load.from();

                                {
                                        lock_guard<mutex> locked(Vertex::g_vinfo.mtx);

                                        if (Vertex::g_vinfo.parent == "")
                                        {
                                                Vertex::g_vinfo.parent = load.from();
                                                Vertex::g_vinfo.expected_msg = Vertex::g_vinfo.neighbors.size() - 1;
                                        } 
                                        
                                        tmp = Vertex::g_vinfo.parent;
                                }

                                
                                if ((Vertex::g_vinfo.neighbors.size() - 1) == 0)
                                {
                                        adi::PayLoad back{};
                                        back.set_from(Vertex::g_vinfo.id);
                                        back.set_type(adi::PayLoad::BACK);
                                        auto* tmp = back.add_pair();
                                        tmp->set_temperature(Vertex::g_vinfo.temperature);
                                        tmp->set_city(Vertex::g_vinfo.city);

                                        Vertex::g_sdr.Submit(load.from(), back.SerializeAsString());

                                        co_return;
                                } 

                                if (tmp == from)
                                {
                                        load.set_from(Vertex::g_vinfo.id);
                                        string bytes = load.SerializeAsString();

                                        for (uint i = 0; i < Vertex::g_vinfo.neighbors.size(); ++i)
                                        {
                                                if (Vertex::g_vinfo.neighbors[i] != from)
                                                {
                                                        Vertex::g_sdr.Submit(Vertex::g_vinfo.neighbors[i], bytes);
                                                }
                                        }

                                } else {
                                        load.set_type(adi::PayLoad::BACK);
                                        load.mutable_pair()->Clear();
                                        Vertex::g_sdr.Submit(load.from(), load.SerializeAsString());
                                }

                                co_return;
                        }

                        if (load.type() == adi::PayLoad::BACK)
                        {
                                uint expect = 0;
                                {
                                        lock_guard<mutex> locked(Vertex::g_vinfo.mtx);
                                        expect = --Vertex::g_vinfo.expected_msg;
                                

                                        if (load.pair_size() != 0)
                                        {   
                                                for (int i = 0; i < load.pair_size(); ++i)
                                                {
                                                        auto* p = Vertex::g_vinfo.store.Add();
                                                        p->set_city(load.pair(i).city());
                                                        p->set_temperature(load.pair(i).temperature());
                                                }
                                        }
                                }

                                if (expect == 0)
                                {
                                        string host{};
                                        {
                                                lock_guard<mutex> locked(Vertex::g_vinfo.mtx);

                                                host = Vertex::g_vinfo.parent;
                                                Vertex::g_vinfo.expected_msg = Vertex::g_vinfo.neighbors.size() - 1;
                                        }
                                        
                                        if (host != "root")
                                        {
                                                adi::PayLoad back{};
                                                back.set_from(Vertex::g_vinfo.id);
                                                back.set_type(adi::PayLoad::BACK);
                                                
                                                {       
                                                        lock_guard<mutex> locked(Vertex::g_vinfo.mtx);

                                                        *back.mutable_pair() = std::move(Vertex::g_vinfo.store);
                                                        Vertex::g_vinfo.store.Clear();
                                                }

                                                auto* p = back.add_pair();
                                                p->set_city(Vertex::g_vinfo.city);
                                                p->set_temperature(Vertex::g_vinfo.temperature);

                                                Vertex::g_sdr.Submit(host, back.SerializeAsString());
                                        } else {
                                                
                                                google::protobuf::RepeatedPtrField<adi::PayLoad_Pair> tmp{};
                                                {
                                                        lock_guard<mutex> locked(Vertex::g_vinfo.mtx);
                                                        tmp = std::move(Vertex::g_vinfo.store);
                                                }
                                                
                                                printf("%s: %d°C\n", Vertex::g_vinfo.city.data(), Vertex::g_vinfo.temperature);
                                                for (auto const& p : tmp)
                                                {
                                                        printf("%s: %d°C\n", p.city().data(), p.temperature());
                                                }
                                        }
                                }

                        }

                        co_return;
                }
        };

public:
        Vertex(const char* host) : m_acpr{host}
        {}

        void set_id(char const* inId)
        {
                g_vinfo.id = inId;
        }

        void set_city(char const* inCity)
        {
                g_vinfo.city = inCity;
        }

        void set_temperature(int inTemp)
        {
                g_vinfo.temperature = inTemp;
        }

        void add_neighbor(char const* inNeighbor)
        {
                g_vinfo.neighbors.emplace_back(inNeighbor);
        }

        void Launch()
        {
                printf("#%s[%s] is launched right now...\n", g_vinfo.city.data(), g_vinfo.id.data());

                m_acpr.Run();
        }


private:
        static inline VertexInfo g_vinfo{};
        static inline Sender g_sdr{};

private:
        Server<A> m_acpr{":1024"};
};