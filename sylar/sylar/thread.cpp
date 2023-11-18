#include "thread.h"
#include "log.h"

namespace sylar {

	static thread_local Thread* t_thread = nullptr;
	static thread_local std::string t_thread_name = "UNKNOW";

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	Thread* Thread::GetThis() {
		return t_thread;
	}

	const std::string& Thread::GetName() {
		return t_thread_name;
	}

	void Thread::SetName(const std::string& name) {
		if (name.empty()) {
			return;
		}
		if (t_thread) {
			t_thread->m_name = name;
		}
		t_thread_name = name;
	}

	Thread::Thread(std::function<void()> cb, const std::string& name)
		:m_cb(cb)
		, m_name(name)
		, m_semaphore{0}{
		if (name.empty()) {
			m_name = "UNKNOW";
		}
		try
		{
			m_thread = std::thread{ &Thread::run,this };
		}
		catch (const std::exception&)
		{
			SYLAR_LOG_ERROR(g_logger) << "create thread fail" << " name=" << name;
			throw std::logic_error("pthread_create error");
		}
		m_semaphore.acquire();
	}

	Thread::~Thread() {
		if (m_thread.joinable()) {
			m_thread.detach();
		}
	}

	void Thread::join() {
		if (m_thread.joinable()) {
			try
			{
				m_thread.join();
			}
			catch (const std::exception&)
			{
				SYLAR_LOG_ERROR(g_logger) << "join thread fail" << " name=" << m_name;
				throw std::logic_error("join error");
			}
		}
	}

	void* Thread::run(void* arg) {
		Thread* thread = (Thread*)arg;
		t_thread = thread;
		t_thread_name = thread->m_name;
		thread->m_id = sylar::GetThreadId();
		//pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

		std::function<void()> cb;
		cb.swap(thread->m_cb);

		thread->m_semaphore.release();

		cb();
		return 0;
	}

}