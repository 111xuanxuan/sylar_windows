#pragma once
/**
 * IOЭ�̵���
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
        * @brief IO�¼�
        */
        enum Event {
            /// ���¼�
            NONE = 0x0,
            /// ���¼�
            READ = 0x1,
            /// д�¼�
            WRITE = 0x4,
        };

    private:

        //Socket�¼���������
        struct FdContext {
            using MutexType = Mutex;
            using Lock = std::lock_guard<MutexType>;
            
            struct EventContext {
                using ptr = std::shared_ptr<EventContext>;

				//�ص��ṹ
				OVERLAPPED overlapped;
                //�¼�ִ�еĵ�����
                Scheduler* scheduler = nullptr;
                //�¼�Э��
                Fiber::ptr fiber;
                //�¼��ص�����
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
            * @brief ��ȡ�¼���������
            * @param[in] event �¼�����
            * @return ���ض�Ӧ�¼���������
            */
            EventContext::ptr getContext(Event event);

            //�����¼�������
            void resetContext(Event event);

            //�����¼�
            void triggerEvent(Event event);




			/// �¼������ľ��
			uintptr_t fd = 0;
			/// ���¼�������
			EventContext::ptr read;
			/// д�¼�������
			EventContext::ptr write;
			/// ��ǰ���¼�
			Event events = Event::NONE;
			/// �¼���Mutex
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
         * @brief �ж��Ƿ����ֹͣ
         * @param[out] timeout ���Ҫ�����Ķ�ʱ���¼����
         * @return �����Ƿ����ֹͣ
         */
        bool stopping(uint64_t& timeout);

    private:

        //IOCP�˿ھ��
        uintptr_t m_iocpfd;

		/// ��ǰ�ȴ�ִ�е��¼�����
		std::atomic<size_t> m_pendingEventCount = { 0 };
		/// IOManager��Mutex
		RWMutexType m_mutex;
		// Socket�¼�������������
        std::unordered_map<uintptr_t, FdContext*> m_fdContexts;
	};




}
#endif 