#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"


namespace sylar {


	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	//协程调度器
	static thread_local Scheduler* t_scheduler = nullptr;

	//协程调度器的调度协程
	static thread_local Fiber* t_scheduler_fiber = nullptr;


	Scheduler::Scheduler(size_t threads /*= 1*/, bool use_caller /*= true*/, const std::string& name /*= ""*/)
		:m_name{name}
	{
		SYLAR_ASSERT(threads > 0);

		//如果当前调度器线程需要负责调度
		if (use_caller) {

			//初始化当前主协程
			sylar::Fiber::GetThis();
			//线程数量减一
			--threads;

			SYLAR_ASSERT(GetThis() == nullptr);

			//设置当前线程的协程调度器
			t_scheduler = this;

			//初始化协程调度器的调度协程
			m_rootFiber.reset(NewFiber(std::bind(&Scheduler::run,this),0,true),FreeFiber);

			//设置当前线程的name
			sylar::Thread::SetName(m_name);

			//设置调度器所在线程的调度协程
			t_scheduler_fiber = m_rootFiber.get();
			//设置调度器的调度协程所在线程的id
			m_rootThread = sylar::GetThreadId();
			//将调度器所在线程id加入
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
		//返回调度器
		return t_scheduler;
	}



	sylar::Fiber* Scheduler::GetMainFiber()
	{
		//返回当前线程的调度器协程
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

		//创建线程
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

		//如果当前线程的调度器是当前这个
		if (Scheduler::GetThis() == this) {

			//是当前线程,直接返回
			if (thread == -1 || thread == sylar::GetThreadId()) {
				return;
			}

		}

		//将当前协程加入调度器,并指定线程
		schedule(Fiber::GetThis(), thread);

		//切换回调度协程
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

		//设置当前线程的调度器
		setThis();

		//当前线程不是调度器所在线程
		if (sylar::GetThreadId() != m_rootThread) {

			//设置当前线程的调度协程
			t_scheduler_fiber = Fiber::GetThis().get();
		}

		//空闲状态下的协程
		Fiber::ptr idle_fiber(NewFiber(std::bind(&Scheduler::idle,this)),FreeFiber);

		//任务协程
		Fiber::ptr cb_fiber;

		//任务
		FiberAndThread ft;

		while (true)
		{
			//清除任务
			ft.reset();

			bool tickle_me = false;
			bool is_active = false;

			{
				WriteLock lock{ m_mutex };

				auto it = m_fibers.begin();

				while (it!=m_fibers.end())
				{
					//判断是否指定某一个线程
					if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
						++it;
						tickle_me = true;
						continue;
					}

					//任务不为空
					SYLAR_ASSERT(it->fiber || it->cb);

					//跳过正在运行中的协程
					if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
						++it;
						continue;
					}

					ft = *it;
					m_fibers.erase(it++);
					//忙碌线程数量加一
					++m_activeThreadCount;
					is_active = true;
					break;
				}

				//如果任务队列不空，发送通知
				tickle_me |= !m_fibers.empty();
			}


			if (tickle_me) {
				tickle();
			}

			//如果是协程且没有异常或者终止
			if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
				//运行协程
				ft.fiber->swapIn();

				--m_activeThreadCount;

				//如果协程状态是Ready,继续加入调度
				if (ft.fiber->getState() == Fiber::READY) {
					schedule(ft.fiber, ft.thread);
				}//如果协程未停止或异常,协程设为暂停状态
				else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
					ft.fiber->m_state = Fiber::HOLD;
				}

				ft.reset();

			}//如果是函数
			else if(ft.cb)
			{
				//如果cb_fiber不为空
				if (cb_fiber) {
					cb_fiber->reset(ft.cb);
				}else{
					//根据这个函数创建一个协程
					cb_fiber.reset(NewFiber(ft.cb),FreeFiber);
				}

				ft.reset();

				//执行协程
				cb_fiber->swapIn();
				//协程退出
				--m_activeThreadCount;

				//如果协程状态为Ready，再次添加调度
				if (cb_fiber->getState() == Fiber::READY) {
					schedule(cb_fiber);
					//释放指针
					cb_fiber.reset();
				}
				else if (cb_fiber->getState() == Fiber::EXCEPT|| cb_fiber->getState() == Fiber::TERM) {
					//重置协程执行函数
					cb_fiber->reset(nullptr);
				}else {
					cb_fiber->m_state = Fiber::HOLD;
					cb_fiber.reset();
				}
			}//其他情况
			else {

				//ft不为空,跳过
				if (is_active) {
					--m_activeThreadCount;
					continue;
				}

				//空闲协程终止
				if (idle_fiber->getState() == Fiber::TERM) {
					SYLAR_LOG_INFO(g_logger) << "idle fiber term";
					break;
				}

				//若任务队列不为空，不进入idle协程
				{
					ReadLock lock{ m_mutex };
					if (!m_fibers.empty()) {
						continue;
					}
				}

				++m_idleThreadCount;
				//切换idle协程
				idle_fiber->swapIn();
				//idle协程退出
				--m_idleThreadCount;

				//如果idle协程不终止或异常，设为暂停
				if (idle_fiber->getState() != Fiber::TERM&&idle_fiber->getState()!=Fiber::EXCEPT) {
					idle_fiber->m_state = Fiber::HOLD;
				}

			}

		}

	}


	bool Scheduler::stopping()
	{
		ReadLock lock{ m_mutex };

		//若任务队列为空且活跃线程数为0则可退出
		return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
	}

	void Scheduler::idle()
	{
		SYLAR_LOG_INFO(g_logger) << "idle";
		//当调度器不停止的时候，挂起返回
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
		//获取当前线程的调度器
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