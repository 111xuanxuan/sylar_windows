#pragma once
/**
 * ��ʱ��
 */
#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.h"
#include "mutex.h"

namespace sylar {

	class TimerManager;

	/**
	 * ��ʱ��
	 */
	class Timer :public std::enable_shared_from_this<Timer> {
		friend class TimerManager;
	public:
		using ptr = std::shared_ptr<Timer>;

		//ȡ����ʱ��
		bool cancel();

		//ˢ�����ö�ʱ����ִ��ʱ��
		bool refresh();

		/**
		* @brief ���ö�ʱ��ʱ��
		 * @param[in] ms ��ʱ��ִ�м��ʱ��(����)
		* @param[in] from_now �Ƿ�ӵ�ǰʱ�俪ʼ����
		*/
		bool reset(uint64_t ms, bool from_now);

	protected:
		/**
		* @brief ���캯��
		* @param[in] ms ��ʱ��ִ�м��ʱ��
		* @param[in] cb �ص�����
		* @param[in] recurring �Ƿ�ѭ��
		* @param[in] manager ��ʱ��������
		*/
		Timer(uint64_t ms, std::function<void()> cb,bool recurring, TimerManager* manager);

		/**
		* @brief ���캯��
		* @param[in] next ִ�е�ʱ���(����)
		*/
		Timer(uint64_t next);

	private:
		/// �Ƿ�ѭ����ʱ��
		bool m_recurring = false;
		/// ִ������
		uint64_t m_ms = 0;
		/// ��ȷ��ִ��ʱ��
		uint64_t m_next = 0;
		/// �ص�����
		std::function<void()> m_cb;
		/// ��ʱ��������
		TimerManager* m_manager = nullptr;

	private:
		/**
		* @brief ��ʱ���ȽϷº���
		*/
		struct Comparator {
			/**
			 * @brief �Ƚ϶�ʱ��������ָ��Ĵ�С(��ִ��ʱ������)
			 * @param[in] lhs ��ʱ������ָ��
			 * @param[in] rhs ��ʱ������ָ��
			 */
			bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
		};

	};


	class TimerManager {
		friend class Timer;
	public:
		using RWMutexType = RWMutex;

		/**
		* @brief ���캯��
		*/
		TimerManager();

		/**
		 * @brief ��������
		 */
		virtual ~TimerManager();

		/**
		* @brief ��Ӷ�ʱ��
		* @param[in] ms ��ʱ��ִ�м��ʱ��
		* @param[in] cb ��ʱ���ص�����
		* @param[in] recurring �Ƿ�ѭ����ʱ��
		*/
		Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
			, bool recurring = false);

		/**
		* @brief ���������ʱ��
		* @param[in] ms ��ʱ��ִ�м��ʱ��
		* @param[in] cb ��ʱ���ص�����
		* @param[in] weak_cond ����
		* @param[in] recurring �Ƿ�ѭ��
		*/
		Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
			, std::weak_ptr<void> weak_cond
			, bool recurring = false);

		/**
		* @brief �����һ����ʱ��ִ�е�ʱ����(����)
		*/
		uint64_t getNextTimer();

		/**
		* @brief ��ȡ��Ҫִ�еĶ�ʱ���Ļص������б�
		* @param[out] cbs �ص���������
		*/
		void listExpiredCb(std::vector<std::function<void()> >& cbs);

		/**
		* @brief �Ƿ��ж�ʱ��
		*/
		bool hasTimer();

	protected:

		/**
		* @brief �����µĶ�ʱ�����뵽��ʱ�����ײ�,ִ�иú���
		*/
		virtual void onTimerInsertedAtFront() = 0;

		/**
		 * @brief ����ʱ����ӵ���������
		 */
		void addTimer(Timer::ptr val, WriteLock& lock);

	private:

		/**
		* @brief ��������ʱ���Ƿ񱻵�����
		*/
		bool detectClockRollover(uint64_t now_ms);

	private:

		RWMutex m_mutex;

		/// ��ʱ������
		std::set<Timer::ptr, Timer::Comparator> m_timers;

		/// �Ƿ񴥷�onTimerInsertedAtFront
		bool m_tickled = false;

		/// �ϴ�ִ��ʱ��
		uint64_t m_previouseTime = 0;
	};


}

#endif