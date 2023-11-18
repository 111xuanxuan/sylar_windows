#pragma once
/**
 * IO协程调度
 */
#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__


#include "scheduler.h"
#include "timer.h"

#include "util.h"
#include "log.h"
#include "macro.h"

namespace sylar {


	class IOManager:public Scheduler,public TimerManager{
	public:
		using ptr = std::shared_ptr<IOManager>;
		using RWMutexType = RWMutex;

        /**
        * @brief IO事件
        */
        enum Event {
            /// 无事件
            NONE = 0x0,
            /// 读事件
            READ = 0x1,
            /// 写事件
            WRITE = 0x4,
        };

    private:

        //Socket事件上下文类
        struct FdContext {
            using MutexType = Mutex;
            using Lock = std::lock_guard<MutexType>;
            
            struct EventContext {
                using ptr = std::shared_ptr<EventContext>;

				//重叠结构
				OVERLAPPED overlapped;
                //事件执行的调度器
                Scheduler* scheduler = nullptr;
                //事件协程
                Fiber::ptr fiber;
                //事件回调函数
                std::function<void()> cb;  

                EventContext() {
                    memset(&overlapped, 0, sizeof(OVERLAPPED));
                    overlapped.hEvent = WSACreateEvent();
                }

                ~EventContext() {
                    WSACloseEvent(overlapped.hEvent);
                }

            };

            FdContext(uintptr_t fd) :fd{ fd }{

            }

            ~FdContext() {

            }

            /**
            * @brief 获取事件上下文类
            * @param[in] event 事件类型
            * @return 返回对应事件的上线文
            */
            EventContext::ptr getContext(Event event);

            //重置事件上下文
            void resetContext(Event event);

            //触发事件
            void triggerEvent(Event event);




			/// 事件关联的句柄
			uintptr_t fd = 0;
			/// 读事件上下文
			EventContext::ptr read;
			/// 写事件上下文
			EventContext::ptr write;
			/// 当前的事件
			Event events = Event::NONE;
			/// 事件的Mutex
			MutexType mutex;
        };

    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

        ~IOManager();


        int addEvent(uintptr_t fd, Event event, FdContext::EventContext::ptr eCtx, std::function<void()> cb = nullptr);

        bool delEvent(uintptr_t fd, Event event);

        bool cancelEvent(uintptr_t fd, Event event);

        bool cancelAll(uintptr_t fd);

        static IOManager* GetThis();

		static FdContext::EventContext::ptr CreateEventContext() {
			return std::make_shared<FdContext::EventContext>();
		}

    protected:
		void tickle() override;
		bool stopping() override;
		void idle() override;
		void onTimerInsertedAtFront() override;


        /**
         * @brief 判断是否可以停止
         * @param[out] timeout 最近要出发的定时器事件间隔
         * @return 返回是否可以停止
         */
        bool stopping(uint64_t& timeout);

    private:

        //IOCP端口句柄
        uintptr_t m_iocpfd;

		/// 当前等待执行的事件数量
		std::atomic<size_t> m_pendingEventCount = { 0 };
		/// IOManager的Mutex
		RWMutexType m_mutex;
		// Socket事件上下文类容器
        std::unordered_map<uintptr_t, FdContext*> m_fdContexts;
	};




}
#endif 