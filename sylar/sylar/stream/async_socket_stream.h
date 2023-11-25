#pragma once
#ifndef __SYLAR_STREAMS_ASYNC_SOCKET_STREAM_H__
#define __SYLAR_STREAMS_ASYNC_SOCKET_STREAM_H__

#include "socket_stream.h"
#include <list>
#include <unordered_map>
#include <boost/any.hpp>

namespace sylar {


	//异步SocketStream类
	class AsyncSocketStream :public SocketStream, public std::enable_shared_from_this<AsyncSocketStream> {
	public:
		using ptr = std::shared_ptr<AsyncSocketStream>;
		using RWMutexType = RWMutex;
		using connect_callback = std::function<bool(AsyncSocketStream::ptr)>;
		using disconnect_callback = std::function<void(AsyncSocketStream::ptr)>;

		AsyncSocketStream(Socket::ptr sock, bool owner = true);

		virtual bool start();
		virtual void close() override;


	public:
		//错误代码枚举
		enum Error {
			OK = 0,
			TIMEOUT = -1,
			IO_ERROR = -2,
			NOT_CONNECT = -3,
		};

	protected:

		struct SendCtx {
		public:
			using ptr= std::shared_ptr<SendCtx>;
			virtual ~SendCtx() {}

			virtual bool doSend(AsyncSocketStream::ptr stream) = 0;
		};

		//上下文
		struct Ctx : public SendCtx {
		public:
			using ptr= std::shared_ptr<Ctx>;
			virtual ~Ctx() {}
			Ctx();

			//序列号
			uint32_t sn;
			uint32_t timeout;
			uint32_t result;
			//是否超时
			bool timed;

			//调度器
			std::atomic<Scheduler*> scheduler;
			Fiber::ptr fiber;
			//定时器
			Timer::ptr timer;

			std::string resultStr = "ok";

			//Response,执行相应的fiber
			virtual void doRsp();
		};

	public:

		void setWorker(sylar::IOManager* v) { m_worker = v; }
		sylar::IOManager* getWorker() const { return m_worker; }

		void setIOManager(sylar::IOManager* v) { m_iomanager = v; }
		sylar::IOManager* getIOManager() const { return m_iomanager; }

		bool isAutoConnect() const { return m_autoConnect; }
		void setAutoConnect(bool v) { m_autoConnect = v; }

		connect_callback getConnectCb() const { return m_connectCb; }
		disconnect_callback getDisconnectCb() const { return m_disconnectCb; }
		void setConnectCb(connect_callback v) { m_connectCb = v; }
		void setDisconnectCb(disconnect_callback v) { m_disconnectCb = v; }

		template<class T>
		void setData(const T& v) { m_data = v; }

		template<class T>
		T getData() const {
			try {
				return boost::any_cast<T>(m_data);
			}
			catch (...) {
			}
			return T();
		}

	protected:
		virtual void doRead();
		virtual void doWrite();
		virtual void startRead();
		virtual void startWrite();
		//超时
		virtual void onTimeOut(Ctx::ptr ctx);
		virtual Ctx::ptr doRecv() = 0;
		virtual void onClose() {}

		Ctx::ptr getCtx(uint32_t sn);
		Ctx::ptr getAndDelCtx(uint32_t sn);

		template<class T>
		std::shared_ptr<T> getCtxAs(uint32_t sn) {
			auto ctx = getCtx(sn);

			if (ctx) {
				return std::dynamic_pointer_cast<T>(ctx);
			}

			return nullptr;
		}

		template<class T>
		std::shared_ptr<T> getAndDelCtxAs(uint32_t sn) {
			auto ctx = getAndDelCtx(sn);
			if (ctx) {
				return std::dynamic_pointer_cast<T>(ctx);
			}
			return nullptr;
		}

		bool addCtx(Ctx::ptr ctx);
		bool enqueue(SendCtx::ptr ctx);

		bool innerClose();
		bool waitFiber();


		protected:
			FiberSemaphore m_sem;
			FiberSemaphore m_waitSem;

			RWMutexType m_queueMutex;
			//发送任务队列
			std::list<SendCtx::ptr> m_queue;

			RWMutexType m_mutex;
			//上下文
			std::unordered_map<uint32_t, Ctx::ptr> m_ctxs;

			uint32_t m_sn;
			//是否自动连接
			bool m_autoConnect;
			uint16_t m_tryConnectCount;
			//定时器
			sylar::Timer::ptr m_timer;
			//io调度器
			sylar::IOManager* m_iomanager;
			//工作调度器
			sylar::IOManager* m_worker;

			//连接回调
			connect_callback m_connectCb;
			//断开连接回调
			disconnect_callback m_disconnectCb;
			//数据
			boost::any m_data;
		public:
			bool recving = false;
	};


	//异步SocketStream管理器
	class AsyncSocketStreamManager {
	public:
		using RWMutexType = RWMutex;
		using connect_callback = AsyncSocketStream::connect_callback;
		using disconnect_callback = AsyncSocketStream::disconnect_callback;

		AsyncSocketStreamManager();
		virtual ~AsyncSocketStreamManager() {}

		void add(AsyncSocketStream::ptr stream);
		void clear();
		void setConnection(const std::vector<AsyncSocketStream::ptr>& streams);
		AsyncSocketStream::ptr get();

		template<class T>
		std::shared_ptr<T> getAs() {
			auto rt = get();
			if (rt) {
				return std::dynamic_pointer_cast<T>(rt);
			}
			return nullptr;
		}

		connect_callback getConnectCb() const { return m_connectCb; }
		disconnect_callback getDisconnectCb() const { return m_disconnectCb; }

		void setConnectCb(connect_callback v);
		void setDisconnectCb(disconnect_callback v);

	private:

		RWMutexType m_mutex;
		uint32_t m_size;
		std::atomic_uint32_t  m_idx;
		std::vector<AsyncSocketStream::ptr> m_datas;
		//连接回调
		connect_callback m_connectCb;
		//断开连接回调
		disconnect_callback m_disconnectCb;

	};


}
#endif