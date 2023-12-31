#pragma once
#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include "config.h"
#include "socket.h"
#include "noncopyable.h"
#include "address.h"
#include "iomanager.h"


namespace sylar {

	//TcpServer配置
	struct TcpServerConf {
		using ptr = std::shared_ptr<TcpServerConf>;

		//服务器监听的地址列表
		std::vector<std::string> address;
		//是否alive
		int keepalive = 0;
		//超时时间
		int timeout = 1000 * 20 * 60;
		//是否启用ssl
		int ssl = 0;
		//服务器唯一标识id
		std::string id;
		/// 服务器类型，http, ws, rock
		std::string type = "http";
		//服务器名称
		std::string name;
		//SSL证书文件路径
		std::string cert_file;
		//SSL密钥文件路径
		std::string key_file;
		// 接受连接的管理器名称
		std::string accept_worker;
		//处理I/O操作的管理器名称
		std::string io_worker;
		//处理业务逻辑的的管理器名称
		std::string process_worker;
		//存储其他配置参数的映射
		std::map<std::string, std::string> args;

		bool isValid()const {
			return !address.empty();
		}

		bool operator==(const TcpServerConf& oth) const {
			return address == oth.address
				&& keepalive == oth.keepalive
				&& timeout == oth.timeout
				&& name == oth.name
				&& ssl == oth.ssl
				&& cert_file == oth.cert_file
				&& key_file == oth.key_file
				&& accept_worker == oth.accept_worker
				&& io_worker == oth.io_worker
				&& process_worker == oth.process_worker
				&& args == oth.args
				&& id == oth.id
				&& type == oth.type;

		}
	};

	//字符串到TcpServerConf的转换
	template<>
	class LexicalCast<std::string, TcpServerConf> {
	public:
		TcpServerConf operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			TcpServerConf conf;
			conf.id = node["id"].as<std::string>(conf.id);
			conf.type = node["type"].as<std::string>(conf.type);
			conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
			conf.timeout = node["timeout"].as<int>(conf.timeout);
			conf.name = node["name"].as<std::string>(conf.name);
			conf.ssl = node["ssl"].as<int>(conf.ssl);
			conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
			conf.key_file = node["key_file"].as<std::string>(conf.key_file);
			conf.accept_worker = node["accept_worker"].as<std::string>();
			conf.io_worker = node["io_worker"].as<std::string>();
			conf.process_worker = node["process_worker"].as<std::string>();
			conf.args = LexicalCast<std::string
				, std::map<std::string, std::string> >()(node["args"].as<std::string>(""));
			if (node["address"].IsDefined()) {
				for (size_t i = 0; i < node["address"].size(); ++i) {
					conf.address.push_back(node["address"][i].as<std::string>());
				}
			}
			return conf;
		}

	};

	//TcpServerConf到字符串的转换
	template<>
	class LexicalCast<TcpServerConf, std::string> {
	public:
		std::string operator()(const TcpServerConf& conf) {
			YAML::Node node;
			node["id"] = conf.id;
			node["type"] = conf.type;
			node["name"] = conf.name;
			node["keepalive"] = conf.keepalive;
			node["timeout"] = conf.timeout;
			node["ssl"] = conf.ssl;
			node["cert_file"] = conf.cert_file;
			node["key_file"] = conf.key_file;
			node["accept_worker"] = conf.accept_worker;
			node["io_worker"] = conf.io_worker;
			node["process_worker"] = conf.process_worker;
			node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>
				, std::string>()(conf.args));
			for (auto& i : conf.address) {
				node["address"].push_back(i);
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};


	//TcpServer类
	class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
	public:
		using ptr = std::shared_ptr<TcpServer>;

		/**
		* @brief 构造函数
		* @param[in] worker socket客户端工作的协程调度器
		* @param[in] accept_worker 服务器socket执行接收socket连接的协程调度器
		*/
		TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis()
			, sylar::IOManager* io_woker = sylar::IOManager::GetThis()
			, sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

		virtual ~TcpServer();

		virtual bool bind(sylar::Address::ptr addr, bool ssl = false);

		virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl = false);

		bool loadCertificates(const std::string& cert_file, const std::string& key_file);

		/**
		* @brief 启动服务
		* @pre 需要bind成功后执行
		*/
		virtual bool start();

		/**
		 * @brief 停止服务
		 */
		virtual void stop();

		/**
		 * @brief 返回读取超时时间(毫秒)
		 */
		uint64_t getRecvTimeout() const { return m_recvTimeout; }

		/**
		 * @brief 返回服务器名称
		 */
		std::string getName() const { return m_name; }

		/**
		 * @brief 设置读取超时时间(毫秒)
		 */
		void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

		/**
		 * @brief 设置服务器名称
		 */
		virtual void setName(const std::string& v) { m_name = v; }

		/**
		 * @brief 是否停止
		 */
		bool isStop() const { return m_isStop; }

		TcpServerConf::ptr getConf() const { return m_conf; }
		void setConf(TcpServerConf::ptr v) { m_conf = v; }
		void setConf(const TcpServerConf& v);

		virtual std::string toString(const std::string& prefix = "");

		std::vector<Socket::ptr> getSocks() const { return m_socks; }
		bool isSSL() const { return m_ssl; }

	protected:
		/**
		* @brief 处理新连接的Socket类
		*/
		virtual void handleClient(Socket::ptr client);

		/**
		 * @brief 开始接受连接
		 */
		virtual void startAccept(Socket::ptr sock);

	protected:
		/// 监听Socket数组
		std::vector<Socket::ptr> m_socks;
		/// 新连接的Socket工作的调度器
		IOManager* m_worker;
		//io调度管理器
		IOManager* m_ioWorker;
		/// 服务器Socket接收连接的调度器
		IOManager* m_acceptWorker;
		/// 接收超时时间(毫秒)
		uint64_t m_recvTimeout;
		/// 服务器名称
		std::string m_name;
		/// 服务器类型
		std::string m_type = "tcp";
		/// 服务是否停止
		bool m_isStop;
		//是否含有ssl
		bool m_ssl = false;
		//服务器配置
		TcpServerConf::ptr m_conf;

	};

}
#endif