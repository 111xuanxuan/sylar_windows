#pragma once
#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include "noncopyable.h"
#include <atomic>
#include <shared_mutex>
#include <list>
#include "fiber.h"
#include "scheduler.h"

namespace sylar {

	using Mutex = std::mutex;
	using RWMutex = std::shared_mutex;
	using ReadLock = std::shared_lock<std::shared_mutex>;
	using WriteLock = std::unique_lock<std::shared_mutex>;


	class Spinlock final:Noncopyable {
	public:
		Spinlock() {
			m_mutex.clear();
		}

		~Spinlock() {
		}

		void lock() {
			while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
		}

		void unlock() {
			std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
		}

	private:
		std::atomic_flag m_mutex;
	};

	using Lock = std::lock_guard<sylar::Spinlock>;

	class Scheduler;
	class FiberSemaphore : Noncopyable {
	public:
		typedef Spinlock MutexType;

		FiberSemaphore(size_t initial_concurrency = 0);
		~FiberSemaphore();

		bool tryWait();
		void wait();
		void notify();
		void notifyAll();

		size_t getConcurrency() const { return m_concurrency; }
		void reset() { m_concurrency = 0; }
	private:
		MutexType m_mutex;
		std::list<std::pair<Scheduler*, Fiber::ptr> > m_waiters;
		size_t m_concurrency;
	};

}


#endif