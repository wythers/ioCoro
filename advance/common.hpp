/*
 * common.h
 * 
 ***************************************************************
 * This program is part of the source code released for 
 *  "Distributed Algorithms Implementation With ioCoro"
 *  (c) Author: wyther yang
 *
 * From:
 *  'Common' code.
 ****************************************************************
 * Brief Description:
 * This is the 'common' code that may get compiled into all the sections
 */
#pragma once

#include <utility>

using std::forward;

using Condi_Idx = std::size_t;

struct null_chain
{
        bool m_end = false;

        void do_complete()
        {}
};

template<Condi_Idx I, typename F, typename Pre, typename D>
class Chain
{
        template<Condi_Idx, typename, typename, typename>
        friend class Chain;

public:
        Chain() = delete;

        Chain(F&& f, Pre* pre, D& data)
        : m_local_task(forward<F>(f))
        , m_pre(pre)
        , m_end(false)
        , m_data(data)
        {
                if (m_pre)
                        m_pre->m_end = true;    
        }

        template<Condi_Idx N, typename Func>
        auto Condition(Func&& f)
        {
                return Chain<N, Func, Chain, D>(forward<Func>(f), this, m_data);
        }

        void do_complete()
        {
                if (m_data.type() == I)
                        m_local_task(m_data);
                else 
                {
                        if (m_pre)
                                m_pre->do_complete();
                }
        }

        ~Chain()
        {
                if (!m_end)
                        do_complete();
        }

private:
        F m_local_task;
        Pre* m_pre;
        bool m_end;

        D& m_data;
};

template<typename D>
struct Anchor
{
        Anchor() = delete;

        Anchor(D& d)
        : m_data(d)
        {}

        template<Condi_Idx N, typename Func>
        auto Condition(Func&& f)
        {
                return Chain<N, Func, null_chain, D>(forward<Func>(f), nullptr, m_data);
        }

        D& m_data;
};
