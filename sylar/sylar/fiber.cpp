#include "fiber.h"
#include "log.h"
#include "config.h"
#include "macro.h"
#include "scheduler.h"

namespace sylar {

	static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	static std::atomic<uint64_t> s_fiber_id{ 0 };
	static std::atomic<uint64_t> s_fiber_count{ 0 };

	//当前协程
	static thread_local Fiber* t_fiber = nullptr;
	//线程的主协程
	static thread_local Fiber::ptr t_threadFiber = nullptr;

	static ConfigVar<uint32_t>::ptr g_fiber_statck_size = Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

	static constexpr size_t MAX_STACK_SIZE = 8 * 1024 * 1024;
	static constexpr size_t MIN_STACK_SIZE = 8 * 1024;


	/**
	 * 栈内存申请器
	 */
	class MallocStackAllocator {
	public:
		static void* Alloc(size_t size) {

			SYLAR_ASSERT(MIN_STACK_SIZE<=size);
			SYLAR_ASSERT(MAX_STACK_SIZE>=size);

			void* limit = malloc(size);
			if (!limit) throw std::bad_alloc();

			return static_cast<char*>(limit) + size;
		}

		static void Dealloc(void* vp, size_t size) {

			SYLAR_ASSERT(vp);
			SYLAR_ASSERT(MIN_STACK_SIZE <= size);
			SYLAR_ASSERT(MAX_STACK_SIZE >= size);

			void* limit = static_cast<char*>(vp) - size;
			free(limit);
		}
	};

	using StackAllocator = MallocStackAllocator;



	uint64_t Fiber::GetFiberId()
	{
		if (t_fiber) {
			return t_fiber->getId();
		}
		return 0;
	}

	sylar::Fiber* NewFiber()
	{
		return new Fiber();
	}

	sylar::Fiber* NewFiber(std::function<void()> cb, size_t stacksize /*= 0*/, bool use_caller /*= false*/)
	{
		stacksize = stacksize ? stacksize : g_fiber_statck_size->getValue();

		Fiber* p = (Fiber*)malloc(sizeof(Fiber));
		return new(p) Fiber(cb, stacksize, use_caller);
	}
	
	void FreeFiber(Fiber* ptr)
	{
		ptr->~Fiber();
		free(ptr);
	}

	Fiber::Fiber()
	{
		m_state = EXEC;
		SetThis(this);
		++s_fiber_count;
		SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
	}

	Fiber::Fiber(std::function<void()> cb, size_t stacksize /*= 0*/, bool use_caller /*= false*/)
		:m_id{++s_fiber_id}
		,m_cb{cb}{

		++s_fiber_count;
		m_stacksize = stacksize;
		m_stack = StackAllocator::Alloc(m_stacksize);

		if (!use_caller) {
			m_ctx=ctx::make_fcontext(m_stack, m_stacksize, &Fiber::MainFunc);
		}
		else {
			m_ctx=ctx::make_fcontext(m_stack, m_stacksize, &Fiber::CallerMainFunc);
		}

		SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;

	}

	Fiber::~Fiber()
	{
		--s_fiber_count;
		if (m_stack) {
			SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
			StackAllocator::Dealloc(m_stack, m_stacksize);
		}
		else
		{
			SYLAR_ASSERT(!m_cb);
			SYLAR_ASSERT(m_state == EXEC);

			Fiber* cur = t_fiber;
			if (cur == this) {
				SetThis(nullptr);
			}
		}

		SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
			<< " total=" << s_fiber_count;

	}

	void Fiber::reset(std::function<void()> cb)
	{

		SYLAR_ASSERT(m_stack);
		SYLAR_ASSERT(m_state == TERM|| m_state == EXCEPT|| m_state == INIT);

		m_cb = cb;

		m_ctx = ctx::make_fcontext(m_stack, m_stacksize, &Fiber::MainFunc);

		m_state = INIT;

	}

	void Fiber::call()
	{
		SetThis(this);
		m_state = EXEC;
		
		transfer_t t=ctx::jump_fcontext(m_ctx,nullptr);
		m_ctx = t.fctx;
	}

	void Fiber::back()
	{
		SetThis(t_threadFiber.get());
		transfer_t t = ctx::jump_fcontext(t_threadFiber->m_ctx, nullptr);
		t_threadFiber->m_ctx = t.fctx;
	}

	void Fiber::swapIn()
	{
		SetThis(this);
		SYLAR_ASSERT(m_state != EXEC);
		m_state = EXEC;

		transfer_t t = ctx::jump_fcontext(m_ctx,0);
		m_ctx = t.fctx;

	}

	void Fiber::swapOut()
	{
		SetThis(Scheduler::GetMainFiber());
		transfer_t t = ctx::jump_fcontext(Scheduler::GetMainFiber()->m_ctx, 0);
		Scheduler::GetMainFiber()->m_ctx = t.fctx;

	}


	void Fiber::SetThis(Fiber* f)
	{
		t_fiber = f;
	}

	sylar::Fiber::Fiber::ptr Fiber::GetThis()
	{
		if(t_fiber) {
			return t_fiber->shared_from_this();
		}

		Fiber::ptr main_fiber(NewFiber());
		SYLAR_ASSERT(t_fiber == main_fiber.get());
		t_threadFiber = main_fiber;
		return t_fiber->shared_from_this();

	}

	void Fiber::YieldToReady()
	{
		Fiber::ptr cur = GetThis();
		SYLAR_ASSERT(cur->m_state == EXEC);
		cur->m_state = READY;
		cur->swapOut();
	}

	void Fiber::YieldToHold()
	{
		Fiber::ptr cur = GetThis();
		SYLAR_ASSERT(cur->m_state == EXEC);
		cur->m_state = HOLD;
		cur->swapOut();
	}

	uint64_t Fiber::TotalFibers()
	{
		return s_fiber_count;
	}

	void Fiber::MainFunc(transfer_t vp)
	{
		Fiber::ptr	cur = GetThis();
		t_threadFiber->m_ctx = vp.fctx;

		SYLAR_ASSERT(cur);

		try
		{
			cur->m_cb();
			cur->m_cb = nullptr;
			cur->m_state = TERM;
		}
		catch (const std::exception& ex)
		{
			cur->m_state = EXCEPT;
			SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
				<< " fiber_id=" << cur->getId()
				<< std::endl
				<< sylar::BacktraceToString();
		}
		catch (...) {
			cur->m_state = EXCEPT;
			SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
				<< " fiber_id=" << cur->getId()
				<< std::endl
				<< sylar::BacktraceToString();
		}

		auto raw_ptr = cur.get();
		cur.reset();
		raw_ptr->swapOut();

		SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

	}

	void Fiber::CallerMainFunc(transfer_t vp)
	{
		t_threadFiber->m_ctx = vp.fctx;
		Fiber::ptr cur = GetThis();
		SYLAR_ASSERT(cur);

		try {
			cur->m_cb();
			cur->m_cb = nullptr;
			cur->m_state = TERM;
		}
		catch (std::exception& ex) {
			cur->m_state = EXCEPT;
			SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
				<< " fiber_id=" << cur->getId()
				<< std::endl
				<< sylar::BacktraceToString();
		}
		catch (...) {
			cur->m_state = EXCEPT;
			SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
				<< " fiber_id=" << cur->getId()
				<< std::endl
				<< sylar::BacktraceToString();
		}

		auto raw_ptr = cur.get();
		cur.reset();
		raw_ptr->back();

		SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

	}

}