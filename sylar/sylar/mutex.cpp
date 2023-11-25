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
		//��������ź�,��һ,���ؿ�ִ��
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
		//�����Ҫ�ź�
		if (m_concurrency > 0u) {
			--m_concurrency;
			return;
		}
		//���򽫵�ǰЭ�̴���ȴ�������
		m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::GetThis()));
	}
	//��ͣ�����ص�����Э��
	Fiber::YieldToHold();
}

void sylar::FiberSemaphore::notify()
{
	Lock lock(m_mutex);
	//�ȴ����в�Ϊ��
	if (!m_waiters.empty()) {
		//ȡ��һ��
		auto next = m_waiters.front();
		m_waiters.pop_front();
		//ִ��
		next.first->schedule(next.second);
	}
	else {
		++m_concurrency;
	}
}

void sylar::FiberSemaphore::notifyAll()
{
	Lock lock(m_mutex);
	//ִ�����е�
	for (auto& i : m_waiters) {
		i.first->schedule(i.second);
	}
	m_waiters.clear();
}
