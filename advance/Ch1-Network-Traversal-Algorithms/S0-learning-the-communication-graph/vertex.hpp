#pragma once

#include "vertex_info.pb.h"

#define THREADS_NUM 4
#define NEED_IOCORO_TIMER
#include <iocoro/iocoro.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <unordered_set>


using namespace ioCoro;
using std::string;
using std::queue;
using std::vector;
using std::unordered_map;

struct Vertex {

static inline mutex g_mtx{};

static inline queue<string> vertexs{};
static inline condition_variable g_condi{};
static inline bool start{};

        struct S {

                static IoCoro Active(Stream stream, char const* host, char const* neighbor, string bytes)
                {
                        unique_stream cleanup(stream);

                        {
                                DeadLine L1([&]{
                                        stream.Close();
                                        printf("#%s: try to connect #%s failed\n", host, neighbor);
                                }, stream, 2s);

                                co_await ioCoroConnect(stream, neighbor);
                        }
                        if (stream)
                                co_return;
                        
                        stream.KeepAlive();
                        for (;;)
                        {
                                {
                                        DeadLine L2([&]{
                                                stream.Close();
                                                printf("#%s: Oops, #%s seems be busy\n", host, neighbor);
                                        }, stream, 2s);
                                        co_await ioCoroCompletedWrite(stream, bytes.data(), bytes.size());
                                }
                                if (stream)
                                {
                                        if (stream.StateCode() == errors::timed_out)
                                        {
                                                DeadLine L3([&]{
                                                        stream.Close();
                                                        printf("#%s: try to reconnect #%s failed\n", host, neighbor);
                                                }, stream, 2s);
                                                co_await ioCoroReconnect(stream, neighbor);
                                        }

                                        if (!stream)
                                                continue;;
                                }

                                co_return;
                        }
                }
        };

        struct A {

                static IoCoro Passive(Stream streaming)
                {
                        unique_stream cleanup(streaming);
                        
                        {
                                lock_guard<mutex> locked(g_mtx);
                                if (!start)
                                {
                                        kill(getpid(), SIGUSR1);
                                        start = true;
                                }
                        }

                        string buf{};
                        buf.resize(128);
                        ssize_t ret{};

                        {
                                DeadLine L1([&]{
                                        streaming.Close();
                                }, streaming, 1s);
                                ret = co_await ioCoroCompletedRead(streaming, buf.data(), 127);
                        }
                        if (streaming)
                                co_return;
                        
                        buf.resize(ret);

                        {
                                lock_guard<mutex> locked(g_mtx);
                                vertexs.push(std::move(buf));
                        }

                        g_condi.notify_one();
                }
        };

        using Sender = Client<S>;
        using Acceptor = Server<A>;

        Vertex() = delete;

        Vertex(char const* id) 
        : m_host_and_neighbors{id, {}}
        , m_acpr{id}
        {
                m_adj.insert({ { m_host_and_neighbors.first }, {} });
        }

        void add_neighbor(char const* n)
        {
                m_host_and_neighbors.second.push_back(n);
                m_adj[m_host_and_neighbors.first].push_back(n);
        }

        void Launch()
        {
                // let ioCoro give up a worker temporarily to stand by
                m_sdr.Submit([&, this]{
                        int s = 0;
                        sigset_t set;
                        sigemptyset(&set);
                        sigaddset(&set, SIGUSR1);

                        adi::VertexInfo self{};
                        self.set_id(m_host_and_neighbors.first);
                        self.set_from(m_host_and_neighbors.first);

                        for (uint i = 0; i < m_host_and_neighbors.second.size(); ++i)
                        {
                                string* tmp = self.add_neighbors();
                                *tmp = m_host_and_neighbors.second[i];
                        }

                        string buf(self.SerializeAsString());

                        if (sigwait(&set, &s))
                        {
                                printf("#%s: START thread failed\n", m_host_and_neighbors.first);
                                return;
                        }
                        
                        {
                                lock_guard<mutex> locked(g_mtx);
                                start = true;
                        }
                        
                        for (char const* neighbor : m_host_and_neighbors.second)
                        {
                                m_sdr.Submit(m_host_and_neighbors.first, neighbor, buf);
                        }

                });

                // let ioCoro give up a worker to do tail task
                m_sdr.Submit([this]{
                        
                        bool done = false;

                        for (;;)
                        {
                                string tmp{};
                                {
                                        unique_lock<mutex> locked(g_mtx);
                                        g_condi.wait(locked, [&]{ return !vertexs.empty(); });

                                        tmp = std::move(vertexs.front());
                                        vertexs.pop();
                                }

                                adi::VertexInfo info{};
                                info.ParseFromString(tmp);
                                string from = info.from();
                                info.set_from(m_host_and_neighbors.first);
                                tmp = info.SerializeAsString();

                                if (!done)
                                {

                                        if (!m_adj.count(info.id()))
                                        {
                                                m_adj[info.id()] = {};
                                                for (int i = 0; i < info.neighbors_size(); ++i)
                                                {
                                                        m_adj[info.id()].push_back(info.neighbors(i));
                                                }

                                                int flag = false;

                                                // simple termination judgment
                                                for (auto const& v : m_adj)
                                                {
                                                        for (auto const& s : v.second)
                                                        {
                                                                if (!m_adj.count(s))
                                                                {
                                                                        flag = true;
                                                                        break;
                                                                }
                                                        }
                                                        
                                                        if (flag)
                                                                break;
                                                }

                                                if (!flag)
                                                {
                                                        done = true;

                                                        string outstr{};
                                                        outstr += "-------------------------------------------\n";
                                                        outstr += string{"Vertex #"} + m_host_and_neighbors.first + ":\n";
                                                        
                                                        for (auto const& v : m_adj)
                                                        {
                                                                outstr += v.first + ":";
                                                                for (auto const& s : v.second)
                                                                {
                                                                        outstr += " " + s;
                                                                }

                                                                outstr += "\n";
                                                        }
                                                        outstr += "-------------------------------------------\n";

                                                        // learning finished, print the adj-table now.
                                                        printf("%s", outstr.data());
                                                }
                                        }
                                }

                                // “when new, forward", "when not new, discard"
                                for (char const* neighbor : m_host_and_neighbors.second)
                                {                 
                                        if (from != string{neighbor})
                                        {
                                                m_sdr.Submit(m_host_and_neighbors.first, neighbor, tmp);
                                        }
                                }
                        }


                });

                m_acpr.Run();

                printf("Vertex #%s in PID(%d) is ready...\n", m_host_and_neighbors.first, getpid());
        }

public:
        unordered_map<string, vector<string>> m_adj{};

        std::pair<char const*, vector<char const*>> m_host_and_neighbors{};

        Sender m_sdr{};
        
        Acceptor m_acpr{":1024"};
};