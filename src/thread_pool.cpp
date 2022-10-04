#include "thread_pool.hpp"

using namespace ioCoro;

TaskPool::TaskPool(int inThreadNum)
{
  for (int i = 0; i < inThreadNum; ++i) {
    m_threads.emplace_back([this] { Run(); });
  }
}

TaskPool::~TaskPool()
{
  for (auto& t : m_threads)
    t.join();
}

void
TaskPool::Run()
{
  while (!IsStoped()) {

    Operation* task{};

    {
      unique_lock<mutex> locked(m_mutex);
      m_condi.wait(locked, [this] { return Condi_state(); });

      if (IsStoped())
        return;

      task = m_ops.Front();
      m_ops.PopFront();
    }
    
    (*task)();
  }
}