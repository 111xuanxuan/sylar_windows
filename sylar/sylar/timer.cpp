#include "timer.h"
#include "util.h"

namespace sylar {

	bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
	{
		if (!lhs && !rhs) {
			return false;
		}

		if (!lhs) {
			return true;
		}

		if (!rhs) {
			return true;
		}

		if (lhs->m_next < rhs->m_next) {
			return true;
		}

		if (rhs->m_next < lhs->m_next) {
			return false;
		}

		return lhs.get() < rhs.get();

	}

	bool Timer::cancel()
	{
		WriteLock lock{m_manager->m_mutex};

		//如果有回调函数
		if (m_cb) {

			m_cb = nullptr;
			auto it = m_manager->m_timers.find(shared_from_this());
			m_manager->m_timers.erase(it);

			return true;

		}

		return false;

	}

	bool Timer::refresh()
	{
		//没有回调函数
		if (!m_cb) {
			return false;
		}

		WriteLock lock{ m_manager->m_mutex };

		auto it = m_manager->m_timers.find(shared_from_this());

		if (it == m_manager->m_timers.end()) {
			return false;
		}

		m_manager->m_timers.erase(it);

		//设置下次执行时间为m_ms后
		m_next = sylar::GetCurrentMS() + m_ms;

		m_manager->m_timers.insert(shared_from_this());

		return false;
	}

	bool Timer::reset(uint64_t ms, bool from_now)
	{
		if (ms == m_ms && !from_now) {
			return true;
		}

		if (!m_cb) {
			return false;
		}

		WriteLock lock{ m_manager->m_mutex };

		auto it = m_manager->m_timers.find(shared_from_this());

		if (it == m_manager->m_timers.end()) {
			return false;
		}

		m_manager->m_timers.erase(it);

		uint64_t start = 0;

		if (from_now) {
			start = GetCurrentMS();
		}
		else
		{
			start = m_next - m_ms;
		}

		m_ms = ms;
		m_next = start + m_ms;

		m_manager->addTimer(shared_from_this(), lock);

		return true;

	}

	Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
		:m_recurring(recurring)
		,m_ms{ms}
		,m_cb{cb}
		,m_manager{manager}{
		m_next = sylar::GetCurrentMS() + m_ms;
	}

	Timer::Timer(uint64_t next):m_next{next}
	{
	}



	TimerManager::TimerManager()
	{
		m_previouseTime = GetCurrentMS();
	}

	TimerManager::~TimerManager()
	{

	}

	sylar::Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring /*= false*/)
	{
		Timer::ptr timer = protected_make_shared<Timer>(ms, cb, recurring, this);
		WriteLock lock{ m_mutex };
		addTimer(timer, lock);
		return timer;
	}

	void TimerManager::addTimer(Timer::ptr val, WriteLock& lock)
	{
		auto it = m_timers.insert(val).first;


		bool at_front = (it == m_timers.begin()) && !m_tickled;

		lock.unlock();

		if (at_front) {
			m_tickled = true;
		}

		if (at_front) {
			onTimerInsertedAtFront();
		}

	}

	bool TimerManager::detectClockRollover(uint64_t now_ms)
	{
		bool rollover = false;

		if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
			rollover = true;
		}

		m_previouseTime = now_ms;

		return rollover;
	}

	static void OnTimer(std::weak_ptr<void> waek_cond, std::function<void()> cb) {

		if (!waek_cond.expired()) {
			cb();
		}
	}

	sylar::Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring /*= false*/)
	{
		return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
	}

	uint64_t TimerManager::getNextTimer()
	{
		m_tickled = false;

		WriteLock lock{ m_mutex };

		if (m_timers.empty()) {
			return ~0ull;
		}

		const Timer::ptr& next = *m_timers.begin();

		uint64_t now_ms = GetCurrentMS();

		if (now_ms >= next->m_next) {
			return 0;
		}else{
			return next->m_next - now_ms;
		}
	}

	void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs)
	{
		uint64_t now_ms = GetCurrentMS();
		std::vector<Timer::ptr> expired;

		{
			ReadLock lock{ m_mutex };
			if (m_timers.empty()) {
				return;
			}
		}

		WriteLock lock{ m_mutex };
		if (m_timers.empty()) {
			return;
		}

		//时间是否调整
		bool rollover = detectClockRollover(now_ms);

		if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
			return;
		}

		//根据当前时间构造定时器
		Timer::ptr now_timer = protected_make_shared<Timer>(now_ms);

		//获取第一个不小于当前定时器的迭代器
		auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);

		//与当前时间相等的包含进去
		while (it!=m_timers.end()&&(*it)->m_next==now_ms)
		{
			++it;
		}

		expired.insert(expired.begin(), m_timers.begin(), it);
		m_timers.erase(m_timers.begin(), it);
		cbs.reserve(expired.size());

		for (auto& timer:expired)
		{
			cbs.push_back(timer->m_cb);
			if (timer->m_recurring) {
				timer->m_next = now_ms + timer->m_ms;
				m_timers.insert(timer);
			}
			else
			{
				timer->m_cb = nullptr;
			}
		}

	}

	bool TimerManager::hasTimer()
	{
		ReadLock lock{ m_mutex };

		return !m_timers.empty();
	}


}