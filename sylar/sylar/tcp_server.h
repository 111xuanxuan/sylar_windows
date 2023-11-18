#pragma once
#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include "config.h"
#include "socket.h"
#include "noncopyable.h"
#include "address.h"
#include "iomanager.h"


namespace sylar {


	struct TcpServerConf {
		using ptr = std::shared_ptr<TcpServerConf>;

		std::vector<std::string> address;
		int keepalive = 0;
		int timeout = 1000 * 20 * 60;
		int ssl = 0;
		std::string id;
		/// ���������ͣ�http, ws, rock
		std::string type = "http";
		std::string name;
		std::string cert_file;
		std::string key_file;
		std::string accept_worker;
		std::string io_worker;
		std::string process_worker;
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

	class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
	public:
		using ptr = std::shared_ptr<TcpServer>;

		/**
		* @brief ���캯��
		* @param[in] worker socket�ͻ��˹�����Э�̵�����
		* @param[in] accept_worker ������socketִ�н���socket���ӵ�Э�̵�����
		*/
		TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis()
			, sylar::IOManager* io_woker = sylar::IOManager::GetThis()
			, sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

		virtual ~TcpServer();

		virtual bool bind(sylar::Address::ptr addr, bool ssl = false);

		virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl = false);

		bool loadCertificates(const std::string& cert_file, const std::string& key_file);

		/**
		* @brief ��������
		* @pre ��Ҫbind�ɹ���ִ��
		*/
		virtual bool start();

		/**
		 * @brief ֹͣ����
		 */
		virtual void stop();

		/**
		 * @brief ���ض�ȡ��ʱʱ��(����)
		 */
		uint64_t getRecvTimeout() const { return m_recvTimeout; }

		/**
		 * @brief ���ط���������
		 */
		std::string getName() const { return m_name; }

		/**
		 * @brief ���ö�ȡ��ʱʱ��(����)
		 */
		void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

		/**
		 * @brief ���÷���������
		 */
		virtual void setName(const std::string& v) { m_name = v; }

		/**
		 * @brief �Ƿ�ֹͣ
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
		* @brief ���������ӵ�Socket��
		*/
		virtual void handleClient(Socket::ptr client);

		/**
		 * @brief ��ʼ��������
		 */
		virtual void startAccept(Socket::ptr sock);

	protected:
		/// ����Socket����
		std::vector<Socket::ptr> m_socks;
		/// �����ӵ�Socket�����ĵ�����
		IOManager* m_worker;
		IOManager* m_ioWorker;
		/// ������Socket�������ӵĵ�����
		IOManager* m_acceptWorker;
		/// ���ճ�ʱʱ��(����)
		uint64_t m_recvTimeout;
		/// ����������
		std::string m_name;
		/// ����������
		std::string m_type = "tcp";
		/// �����Ƿ�ֹͣ
		bool m_isStop;

		bool m_ssl = false;

		TcpServerConf::ptr m_conf;

	};

}
#endif