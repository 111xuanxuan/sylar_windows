#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"


namespace sylar {


	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	//Э�̵�����
	static thread_local Scheduler* t_scheduler = nullptr;

	//Э�̵������ĵ���Э��
	static thread_local Fiber* t_scheduler_fiber = nullptr;


	Scheduler::Scheduler(size_t threads /*= 1*/, bool use_caller /*= true*/, const std::string& name /*= ""*/)
		:m_name{name}
	{
		SYLAR_ASSERT(threads > 0);

		//�����ǰ�������߳���Ҫ�������
		if (use_caller) {

			//��ʼ����ǰ��Э��
			sylar::Fiber::GetThis();
			//�߳�������һ
			--threads;

			SYLAR_ASSERT(GetThis() == nullptr);

			//���õ�ǰ�̵߳�Э�̵�����
			t_scheduler = this;

			//��ʼ��Э�̵������ĵ���Э��
			m_rootFiber.reset(NewFiber(std::bind(&Scheduler::run,this),0,true),FreeFiber);

			//���õ�ǰ�̵߳�name
			sylar::Thread::SetName(m_name);

			//���õ����������̵߳ĵ���Э��
			t_scheduler_fiber = m_rootFiber.get();
			//���õ������ĵ���Э�������̵߳�id
			m_rootThread = sylar::GetThreadId();
			//�������������߳�id����
			m_threadIds.push_back(m_rootThread);

		}
		else
		{
			m_rootThread = -1;
		}
		m_threadCount = threads;
	}

	Scheduler::~Scheduler()
	{
		SYLAR_ASSERT(m_stopping);

		if (GetThis() == this) {
			t_scheduler = nullptr;
		}
	}

	Scheduler* Scheduler::GetThis()
	{
		//���ص�����
		return t_scheduler;
	}



	sylar::Fiber* Scheduler::GetMainFiber()
	{
		//���ص�ǰ�̵߳ĵ�����Э��
		return t_scheduler_fiber;
	}



	void Scheduler::start()
	{
		WriteLock lock{ m_mutex };

		if (!m_stopping) {
			return;
		}

		m_stopping = false;

		SYLAR_ASSERT(m_threads.empty());

		m_threads.resize(m_threadCount);

		//�����߳�
		for (size_t i=0;i<m_threadCount;++i)
		{
			m_threads[i] = std::make_shared<Thread>(std::bind(&Scheduler::run,this),m_name+"_"+std::to_string(i));
			m_threadIds.push_back(m_threads[i]->getId());
		}


	}

	void Scheduler::stop()
	{
		m_autoStop = true;


		if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
			SYLAR_LOG_INFO(g_logger) << this << " stopped";
			m_stopping = true;

			if (stopping()) {
				return;
			}
		}

		if (m_rootThread != -1) {
			SYLAR_ASSERT(GetThis() == this);
		}
		else
		{
			SYLAR_ASSERT(GetThis() != this);
		}

		m_stopping = true;
		for (size_t i=0;i<m_threadCount;++i)
		{
			tickle();
		}

		if (m_rootFiber) {
			tickle();
		}

		if (m_rootFiber) {
			if (!stopping()) {
				m_rootFiber->call();
			}
		}

		std::vector<Thread::ptr> thrs;

		{
			WriteLock lock{ m_mutex };
			thrs.swap(m_threads);
		}

		for (auto& i:thrs)
		{
			i->join();
		}

	}



	void Scheduler::switchTo(int thread /*= -1*/)
	{
		SYLAR_ASSERT(Scheduler::GetThis() != nullptr);

		//�����ǰ�̵߳ĵ������ǵ�ǰ���
		if (Scheduler::GetThis() == this) {

			//�ǵ�ǰ�߳�,ֱ�ӷ���
			if (thread == -1 || thread == sylar::GetThreadId()) {
				return;
			}

		}

		//����ǰЭ�̼��������,��ָ���߳�
		schedule(Fiber::GetThis(), thread);

		//�л��ص���Э��
		Fiber::YieldToHold();
	}

	std::ostream& Scheduler::dump(std::ostream& os)
	{
		os << "[Scheduler name=" << m_name
			<< " size=" << m_threadCount
			<< " active_count=" << m_activeThreadCount
			<< " idle_count=" << m_idleThreadCount
			<< " stopping=" << m_stopping
			<< " ]" << std::endl << "    ";

		for (size_t i = 0; i < m_threadIds.size(); ++i) {
			if (i) {
				os << ", ";
			}
			os << m_threadIds[i];
		}
		return os;
	}

	void Scheduler::tickle()
	{
		SYLAR_LOG_INFO(g_logger) << "tickle";
	}

	void Scheduler::run()
	{
		SYLAR_LOG_DEBUG(g_logger) << m_name << " run";

		set_hook_enable(true);

		//���õ�ǰ�̵߳ĵ�����
		setThis();

		//��ǰ�̲߳��ǵ����������߳�
		if (sylar::GetThreadId() != m_rootThread) {

			//���õ�ǰ�̵߳ĵ���Э��
			t_scheduler_fiber = Fiber::GetThis().get();
		}

		//����״̬�µ�Э��
		Fiber::ptr idle_fiber(NewFiber(std::bind(&Scheduler::idle,this)),FreeFiber);

		//����Э��
		Fiber::ptr cb_fiber;

		//����
		FiberAndThread ft;

		while (true)
		{
			//�������
			ft.reset();

			bool tickle_me = false;
			bool is_active = false;

			{
				WriteLock lock{ m_mutex };

				auto it = m_fibers.begin();

				while (it!=m_fibers.end())
				{
					//�ж��Ƿ�ָ��ĳһ���߳�
					if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
						++it;
						tickle_me = true;
						continue;
					}

					//����Ϊ��
					SYLAR_ASSERT(it->fiber || it->cb);

					//�������������е�Э��
					if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
						++it;
						continue;
					}

					ft = *it;
					m_fibers.erase(it++);
					//æµ�߳�������һ
					++m_activeThreadCount;
					is_active = true;
					break;
				}

				//���������в��գ�����֪ͨ
				tickle_me |= !m_fibers.empty();
			}


			if (tickle_me) {
				tickle();
			}

			//�����Э����û���쳣������ֹ
			if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
				//����Э��
				ft.fiber->swapIn();

				--m_activeThreadCount;

				//���Э��״̬��Ready,�����������
				if (ft.fiber->getState() == Fiber::READY) {
					schedule(ft.fiber, ft.thread);
				}//���Э��δֹͣ���쳣,Э����Ϊ��ͣ״̬
				else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
					ft.fiber->m_state = Fiber::HOLD;
				}

				ft.reset();

			}//����Ǻ���
			else if(ft.cb)
			{
				//���cb_fiber��Ϊ��
				if (cb_fiber) {
					cb_fiber->reset(ft.cb);
				}else{
					//���������������һ��Э��
					cb_fiber.reset(NewFiber(ft.cb),FreeFiber);
				}

				ft.reset();

				//ִ��Э��
				cb_fiber->swapIn();
				//Э���˳�
				--m_activeThreadCount;

				//���Э��״̬ΪReady���ٴ���ӵ���
				if (cb_fiber->getState() == Fiber::READY) {
					schedule(cb_fiber);
					//�ͷ�ָ��
					cb_fiber.reset();
				}
				else if (cb_fiber->getState() == Fiber::EXCEPT|| cb_fiber->getState() == Fiber::TERM) {
					//����Э��ִ�к���
					cb_fiber->reset(nullptr);
				}else {
					cb_fiber->m_state = Fiber::HOLD;
					cb_fiber.reset();
				}
			}//�������
			else {

				//ft��Ϊ��,����
				if (is_active) {
					--m_activeThreadCount;
					continue;
				}

				//����Э����ֹ
				if (idle_fiber->getState() == Fiber::TERM) {
					SYLAR_LOG_INFO(g_logger) << "idle fiber term";
					break;
				}

				//��������в�Ϊ�գ�������idleЭ��
				{
					ReadLock lock{ m_mutex };
					if (!m_fibers.empty()) {
						continue;
					}
				}

				++m_idleThreadCount;
				//�л�idleЭ��
				idle_fiber->swapIn();
				//idleЭ���˳�
				--m_idleThreadCount;

				//���idleЭ�̲���ֹ���쳣����Ϊ��ͣ
				if (idle_fiber->getState() != Fiber::TERM&&idle_fiber->getState()!=Fiber::EXCEPT) {
					idle_fiber->m_state = Fiber::HOLD;
				}

			}

		}

	}


	bool Scheduler::stopping()
	{
		ReadLock lock{ m_mutex };

		//���������Ϊ���һ�Ծ�߳���Ϊ0����˳�
		return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
	}

	void Scheduler::idle()
	{
		SYLAR_LOG_INFO(g_logger) << "idle";
		//����������ֹͣ��ʱ�򣬹��𷵻�
		while (!stopping()) {
			sylar::Fiber::YieldToHold();
		}
	}

	void Scheduler::setThis()
	{
		t_scheduler = this;
	}


	SchedulerSwitcher::SchedulerSwitcher(Scheduler* target /*= nullptr*/)
	{
		//��ȡ��ǰ�̵߳ĵ�����
		m_caller = Scheduler::GetThis();

		if (target) {
			target->switchTo();
		}
	}

	SchedulerSwitcher::~SchedulerSwitcher()
	{
		if (m_caller) {
			m_caller->switchTo();
		}
	}

}