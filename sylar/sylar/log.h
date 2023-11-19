#pragma once
#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>
#include "mutex.h"
#include <list>
#include <map>
#include "singleton.h"
#include <fstream>
#include <boost/lexical_cast.hpp>
#include "util.h"
#include "thread.h"




/**
 * @brief ʹ����ʽ��ʽ����־����level����־д�뵽logger
 */
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)


/**
 * @brief ʹ�ø�ʽ����ʽ����־����level����־д�뵽logger
 */
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)


#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar {

	class Logger;
	class LoggerManager;

	/**
	 * @brief ��־����
	 */
	class LogLevel {
	public:
		/**
		 * @brief ��־����ö��
		 */
		enum Level {
			/// δ֪����
			UNKNOW = 0,
			/// DEBUG ����
			DEBUG = 1,
			/// INFO ����
			INFO = 2,
			/// WARN ����
			WARN = 3,

#ifdef ERROR
#undef ERROR
			/// ERROR ����
			ERROR = 4,

#endif // ERROR

			/// FATAL ����
			FATAL = 5
		};

		/**
		 * @brief ����־����ת���ı����
		 * @param[in] level ��־����
		 */
		static const char* ToString(LogLevel::Level level);

		/**
		 * @brief ���ı�ת������־����
		 * @param[in] str ��־�����ı�
		 */
		static LogLevel::Level FromString(const std::string& str);
	};

	/**
	 * @brief ��־�¼�
	 */
	class LogEvent {
	public:
		using ptr = std::shared_ptr<LogEvent>;
		/**
		 * @brief ���캯��
		 * @param[in] logger ��־��
		 * @param[in] level ��־����
		 * @param[in] file �ļ���
		 * @param[in] line �ļ��к�
		 * @param[in] elapse �������������ĺ�ʱ(����)
		 * @param[in] thread_id �߳�id
		 * @param[in] fiber_id Э��id
		 * @param[in] time ��־�¼�(��)
		 * @param[in] thread_name �߳�����
		 */
		LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
			, const char* file, int32_t line, uint32_t elapse
			, uint32_t thread_id, uint32_t fiber_id, uint64_t time
			, const std::string& thread_name);

		/**
		 * @brief �����ļ���
		 */
		const char* getFile() const { return m_file; }

		/**
		 * @brief �����к�
		 */
		int32_t getLine() const { return m_line; }

		/**
		 * @brief ���غ�ʱ
		 */
		uint32_t getElapse() const { return m_elapse; }

		/**
		 * @brief �����߳�ID
		 */
		uint32_t getThreadId() const { return m_threadId; }

		/**
		 * @brief ����Э��ID
		 */
		uint32_t getFiberId() const { return m_fiberId; }

		/**
		 * @brief ����ʱ��
		 */
		uint64_t getTime() const { return m_time; }

		/**
		 * @brief �����߳�����
		 */
		const std::string& getThreadName() const { return m_threadName; }

		/**
		 * @brief ������־����
		 */
		std::string getContent() const { return m_ss.str(); }

		/**
		 * @brief ������־��
		 */
		std::shared_ptr<Logger> getLogger() const { return m_logger; }

		/**
		 * @brief ������־����
		 */
		LogLevel::Level getLevel() const { return m_level; }

		/**
		 * @brief ������־�����ַ�����
		 */
		std::stringstream& getSS() { return m_ss; }

		/**
		 * @brief ��ʽ��д����־����
		 */
		void format(const char* fmt, ...);

		/**
		 * @brief ��ʽ��д����־����
		 */
		void format(const char* fmt, va_list al);
	private:
		/// �ļ���
		const char* m_file = nullptr;
		/// �к�
		int32_t m_line = 0;
		/// ����������ʼ�����ڵĺ�����
		uint32_t m_elapse = 0;
		/// �߳�ID
		uint32_t m_threadId = 0;
		/// Э��ID
		uint32_t m_fiberId = 0;
		/// ʱ���
		uint64_t m_time = 0;
		/// �߳�����
		std::string m_threadName;
		/// ��־������
		std::stringstream m_ss;
		/// ��־��
		std::shared_ptr<Logger> m_logger;
		/// ��־�ȼ�
		LogLevel::Level m_level;
	};

	/**
	 * @brief ��־�¼���װ��
	 */
	class LogEventWrap {
	public:

		/**
		 * @brief ���캯��
		 * @param[in] e ��־�¼�
		 */
		LogEventWrap(LogEvent::ptr e);

		/**
		 * @brief ��������
		 */
		~LogEventWrap();

		/**
		 * @brief ��ȡ��־�¼�
		 */
		LogEvent::ptr getEvent() const { return m_event; }

		/**
		 * @brief ��ȡ��־������
		 */
		std::stringstream& getSS() {
			return m_event->getSS();
		}
	private:
		/**
		 * @brief ��־�¼�
		 */
		LogEvent::ptr m_event;
	};

	/**
	 * @brief ��־��ʽ��
	 */
	class LogFormatter {
	public:
		using ptr = std::shared_ptr<LogFormatter>;
		/**
		 * @brief ���캯��
		 * @param[in] pattern ��ʽģ��
		 * @details
		 *  %m ��Ϣ
		 *  %p ��־����
		 *  %r �ۼƺ�����
		 *  %c ��־����
		 *  %t �߳�id
		 *  %n ����
		 *  %d ʱ��
		 *  %f �ļ���
		 *  %l �к�
		 *  %T �Ʊ��
		 *  %F Э��id
		 *  %N �߳�����
		 *
		 *  Ĭ�ϸ�ʽ "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
		 */
		LogFormatter(const std::string& pattern);

		/**
		 * @brief ���ظ�ʽ����־�ı�
		 * @param[in] logger ��־��
		 * @param[in] level ��־����
		 * @param[in] event ��־�¼�
		 */
		std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
		std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
	public:

		/**
		 * @brief ��־�������ʽ��
		 */
		class FormatItem {
		public:
			using ptr = std::shared_ptr<FormatItem>;
			/**
			 * @brief ��������
			 */
			virtual ~FormatItem() {}
			/**
			 * @brief ��ʽ����־����
			 * @param[in, out] os ��־�����
			 * @param[in] logger ��־��
			 * @param[in] level ��־�ȼ�
			 * @param[in] event ��־�¼�
			 */
			virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		};

		/**
		 * @brief ��ʼ��,������־ģ��
		 */
		void init();

		/**
		 * @brief �Ƿ��д���
		 */
		bool isError() const { return m_error; }

		/**
		 * @brief ������־ģ��
		 */
		const std::string getPattern() const { return m_pattern; }
	private:
		/// ��־��ʽģ��
		std::string m_pattern;
		/// ��־��ʽ�������ʽ
		std::vector<FormatItem::ptr> m_items;
		/// �Ƿ��д���
		bool m_error = false;
	};

	/**
	 * @brief ��־���Ŀ��
	 */
	class LogAppender {
		friend class Logger;
	public:
		using ptr = std::shared_ptr<LogAppender>;
		using MutexType = sylar::Spinlock;

		/**
		 * @brief ��������
		 */
		virtual ~LogAppender() {}

		/**
		 * @brief д����־
		 * @param[in] logger ��־��
		 * @param[in] level ��־����
		 * @param[in] event ��־�¼�
		 */
		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

		/**
		 * @brief ����־���Ŀ�������ת��YAML String
		 */
		virtual std::string toYamlString() = 0;

		/**
		 * @brief ������־��ʽ��
		 */
		void setFormatter(LogFormatter::ptr val);

		/**
		 * @brief ��ȡ��־��ʽ��
		 */
		LogFormatter::ptr getFormatter();

		/**
		 * @brief ��ȡ��־����
		 */
		LogLevel::Level getLevel() const { return m_level; }

		/**
		 * @brief ������־����
		 */
		void setLevel(LogLevel::Level val) { m_level = val; }
	protected:
		/// ��־����
		LogLevel::Level m_level = LogLevel::DEBUG;
		/// �Ƿ����Լ�����־��ʽ��
		bool m_hasFormatter = false;
		/// Mutex
		MutexType m_mutex;
		/// ��־��ʽ��
		LogFormatter::ptr m_formatter;
	};

	/**
	 * @brief ��־��
	 */
	class Logger : public std::enable_shared_from_this<Logger> {
		friend class LoggerManager;
	public:
		using ptr = std::shared_ptr<Logger>;
		using MutexType = sylar::Spinlock;

		/**
		 * @brief ���캯��
		 * @param[in] name ��־������
		 */
		Logger(const std::string& name = "root");

		/**
		 * @brief д��־
		 * @param[in] level ��־����
		 * @param[in] event ��־�¼�
		 */
		void log(LogLevel::Level level, LogEvent::ptr event);

		/**
		 * @brief дdebug������־
		 * @param[in] event ��־�¼�
		 */
		void debug(LogEvent::ptr event);

		/**
		 * @brief дinfo������־
		 * @param[in] event ��־�¼�
		 */
		void info(LogEvent::ptr event);

		/**
		 * @brief дwarn������־
		 * @param[in] event ��־�¼�
		 */
		void warn(LogEvent::ptr event);

		/**
		 * @brief дerror������־
		 * @param[in] event ��־�¼�
		 */
		void error(LogEvent::ptr event);

		/**
		 * @brief дfatal������־
		 * @param[in] event ��־�¼�
		 */
		void fatal(LogEvent::ptr event);

		/**
		 * @brief �����־Ŀ��
		 * @param[in] appender ��־Ŀ��
		 */
		void addAppender(LogAppender::ptr appender);

		/**
		 * @brief ɾ����־Ŀ��
		 * @param[in] appender ��־Ŀ��
		 */
		void delAppender(LogAppender::ptr appender);

		/**
		 * @brief �����־Ŀ��
		 */
		void clearAppenders();

		/**
		 * @brief ������־����
		 */
		LogLevel::Level getLevel() const { return m_level; }

		/**
		 * @brief ������־����
		 */
		void setLevel(LogLevel::Level val) { m_level = val; }

		/**
		 * @brief ������־����
		 */
		const std::string& getName() const { return m_name; }

		/**
		 * @brief ������־��ʽ��
		 */
		void setFormatter(LogFormatter::ptr val);

		/**
		 * @brief ������־��ʽģ��
		 */
		void setFormatter(const std::string& val);

		/**
		 * @brief ��ȡ��־��ʽ��
		 */
		LogFormatter::ptr getFormatter();

		/**
		 * @brief ����־��������ת��YAML String
		 */
		std::string toYamlString();
	private:
		/// ��־����
		std::string m_name;
		/// ��־����
		LogLevel::Level m_level;
		/// Mutex
		MutexType m_mutex;
		/// ��־Ŀ�꼯��
		std::list<LogAppender::ptr> m_appenders;
		/// ��־��ʽ��
		LogFormatter::ptr m_formatter;
		/// ����־��
		Logger::ptr m_root;
	};

	/**
	 * @brief ���������̨��Appender
	 */
	class StdoutLogAppender : public LogAppender {
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		std::string toYamlString() override;
	};

	/**
	 * @brief ������ļ���Appender
	 */
	class FileLogAppender : public LogAppender {
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		FileLogAppender(const std::string& filename);
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		std::string toYamlString() override;

		/**
		 * @brief ���´���־�ļ�
		 * @return �ɹ�����true
		 */
		bool reopen();
	private:
		/// �ļ�·��
		std::string m_filename;
		/// �ļ���
		std::ofstream m_filestream;
		/// �ϴ����´�ʱ��
		uint64_t m_lastTime = 0;
	};

	/**
	 * @brief ��־��������
	 */
	class LoggerManager {
	public:
		typedef Spinlock MutexType;
		/**
		 * @brief ���캯��
		 */
		LoggerManager();

		/**
		 * @brief ��ȡ��־��
		 * @param[in] name ��־������
		 */
		Logger::ptr getLogger(const std::string& name);

		/**
		 * @brief ��ʼ��
		 */
		void init();

		/**
		 * @brief ��������־��
		 */
		Logger::ptr getRoot() const { return m_root; }

		/**
		 * @brief �����е���־������ת��YAML String
		 */
		std::string toYamlString();
	private:
		/// Mutex
		MutexType m_mutex;
		/// ��־������
		std::map<std::string, Logger::ptr> m_loggers;
		/// ����־��
		Logger::ptr m_root;
	};

	/// ��־�������൥��ģʽ
	typedef sylar::Singleton<LoggerManager> LoggerMgr;
}

#endif
