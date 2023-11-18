#pragma once
/**
 * Э�̵�����
 */

#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "fiber.h"
#include "thread.h"
#include "mutex.h"



namespace sylar {

	/**
	* @brief Э�̵�����
	 * @details ��װ����N-M��Э�̵�����
	*          �ڲ���һ���̳߳�,֧��Э�����̳߳������л�
	*/
	class Scheduler {
	public:
		using ptr = std::shared_ptr<Scheduler>;
		using RWMutexType = RWMutex;

		/**
		* @brief ���캯��
		* @param[in] threads �߳�����
		* @param[in] use_caller �Ƿ�ʹ�õ�ǰ�����߳�
		* @param[in] name Э�̵���������
		*/
		Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

		virtual ~Scheduler();

		/**
		* @brief ����Э�̵���������
		*/
		const std::string& getName() const { return m_name; }

		/**
		* @brief ���ص�ǰЭ�̵�����
		*/
		static Scheduler* GetThis();

		/**
		* @brief ���ص�ǰЭ�̵������ĵ���Э��
		*/
		static Fiber* GetMainFiber();

		/**
		* @brief ����Э�̵�����
		*/
		void start();

		/**
		* @brief ֹͣЭ�̵�����
		*/
		void stop();

		/**
		* @brief ����Э��
		* @param[in] fc Э�̻���
		* @param[in] thread Э��ִ�е��߳�id,-1��ʶ�����߳�
		 */
		template<typename FiberOrcb>
		void schedule(FiberOrcb fc, int thread = -1) {
			bool need_tickle = false;

			{
				WriteLock lock{ m_mutex };
				need_tickle = scheduleNoLock(fc, thread);
			}

			if (need_tickle) {
				tickle();
			}
		}

		/**
		* @brief ��������Э��
		* @param[in] begin Э������Ŀ�ʼ
		* @param[in] end Э������Ľ���
		*/
		template<typename InputIterator>
		void schedule(InputIterator begin, InputIterator end) {
			bool need_tickle = false;

			{
				WriteLock lock{ m_mutex };
				while (begin!=end)
				{
					need_tickle = scheduleNoLock(&*begin, -1)||need_tickle;
					++begin;
				}
			}

			if (need_tickle) {
				tickle();
			}

		}


		void switchTo(int thread = -1);
		std::ostream& dump(std::ostream& os);

	protected:
		/**
		* @brief ֪ͨЭ�̵�������������
		*/
		virtual void tickle();

		/**
		* @brief Э�̵��Ⱥ���
		*/
		void run();

		/**
		 * @brief �����Ƿ����ֹͣ
		 */
		virtual bool stopping();

		/**
		* @brief Э��������ɵ���ʱִ��idleЭ��
		*/
		virtual void idle();

		/**
		 * @brief ���õ�ǰ��Э�̵�����
		 */
		void setThis();


		/**
		* @brief �Ƿ��п����߳�
		*/
		bool hasIdleThreads() { return m_idleThreadCount > 0; }

	private:
		/**
		* @brief Э�̵�������(����)
		*/
		template<typename FiberOrcb>
		bool scheduleNoLock(FiberOrcb fc, int thread) {
			bool need_tickle = m_fibers.empty();
			FiberAndThread ft(fc, thread);
			if (ft.fiber || ft.cb) {
				m_fibers.push_back(ft);
			}
			return need_tickle;
		}

	private:

		struct FiberAndThread {

			/// Э��
			Fiber::ptr fiber;
			/// Э��ִ�к���
			std::function<void()> cb;
			/// �߳�id
			int thread;

			FiberAndThread(Fiber::ptr f, int thr) 
				:fiber{f},thread{thr}{
			}

			FiberAndThread(Fiber::ptr* f, int thr)
				:thread{ thr } {
				fiber.swap(*f);
			}

			FiberAndThread(std::function<void()> f, int thr)
				:cb(f), thread(thr) {
			}

			FiberAndThread(std::function<void()>* f, int thr)
				:thread(thr) {
				cb.swap(*f);
			}

			FiberAndThread()
				:thread(-1) {
			}

			void reset() {
				fiber = nullptr;
				cb = nullptr;
				thread = -1;
			}

		};


	protected:
		//Э���µ��߳�id����
		std::vector<int> m_threadIds;
		size_t m_threadCount = 0;
		/// �����߳�����
		std::atomic<size_t> m_activeThreadCount = { 0 };
		/// �����߳�����
		std::atomic<size_t> m_idleThreadCount = { 0 };
		/// �Ƿ�����ֹͣ
		bool m_stopping = true;
		/// �Ƿ��Զ�ֹͣ
		bool m_autoStop = false;
		/// ���߳�id(use_caller)
		int m_rootThread = 0;
	private:

		RWMutexType m_mutex;
		//�̳߳�
		std::vector<Thread::ptr> m_threads;
		std::list<FiberAndThread> m_fibers;
		/// use_callerΪtrueʱ��Ч, ����Э��
		Fiber::ptr m_rootFiber;
		/// Э�̵���������
		std::string m_name;


	};

	class SchedulerSwitcher :public Noncopyable {
	public:

		SchedulerSwitcher(Scheduler* target = nullptr);

		~SchedulerSwitcher();

	private:
		Scheduler* m_caller;

	};


}



#endif