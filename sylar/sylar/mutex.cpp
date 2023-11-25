#include "mutex.h"
#include "macro.h"
#include "scheduler.h"

sylar::FiberSemaphore::FiberSemaphore(size_t initial_concurrency /*= 0*/):m_concurrency{initial_concurrency}
{
	SYLAR_ASSERT(m_waiters.empty());
}

sylar::FiberSemaphore::~FiberSemaphore()
{
	SYLAR_ASSERT(m_waiters.empty());
}

bool sylar::FiberSemaphore::tryWait()
{
	SYLAR_ASSERT(Scheduler::GetThis());

	{
		Lock lock(m_mutex);
		//如果还有信号,减一,返回可执行
		if (m_concurrency > 0u) {
			--m_concurrency;
			return true;
		}
		return false;
	}
}

void sylar::FiberSemaphore::wait()
{
	SYLAR_ASSERT(Scheduler::GetThis());
	{
		Lock lock(m_mutex);
		//如果还要信号
		if (m_concurrency > 0u) {
			--m_concurrency;
			return;
		}
		//否则将当前协程存入等待队列中
		m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::GetThis()));
	}
	//暂停并返回到调度协程
	Fiber::YieldToHold();
}

void sylar::FiberSemaphore::notify()
{
	Lock lock(m_mutex);
	//等待队列不为空
	if (!m_waiters.empty()) {
		//取出一个
		auto next = m_waiters.front();
		m_waiters.pop_front();
		//执行
		next.first->schedule(next.second);
	}
	else {
		++m_concurrency;
	}
}

void sylar::FiberSemaphore::notifyAll()
{
	Lock lock(m_mutex);
	//执行所有的
	for (auto& i : m_waiters) {
		i.first->schedule(i.second);
	}
	m_waiters.clear();
}
