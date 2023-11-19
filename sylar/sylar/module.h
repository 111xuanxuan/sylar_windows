#pragma once
#ifndef __SYLAR_MODULE_H__
#define __SYLAR_MODULE_H__



#include "stream.h"
#include "singleton.h"
#include "mutex.h"
#include "rock/rock_stream.h"
#include "rock/rock_protocol.h"

namespace sylar {

	class Module {
	public:
		using ptr = std::shared_ptr<Module>;

		enum Type {
			MODULE=0,
			ROCK=1
		};


		Module(const std::string& name, const std::string& version, const std::string& filename, uint32_t type = MODULE);
		
		virtual	~Module(){}

		//
		virtual void onBeforeArgsParse(int argc, char** argv);
		virtual void onAfterArgsParse(int argc, char** argv);

		//处理加载和卸载的生命周期
		virtual bool onLoad();
		virtual bool onUnload();

		//处理连接和断开连接
		virtual bool onConnect(sylar::Stream::ptr stream);
		virtual bool onDisconnect(sylar::Stream::ptr stream);

		//处理服务器状态事件
		virtual bool onServerReady();
		virtual bool onServerUp();

		//处理请求和通知消息
		virtual bool handleRequest(sylar::Message::ptr req
			, sylar::Message::ptr rsp
			, sylar::Stream::ptr stream);
		virtual bool handleNotify(sylar::Message::ptr notify
			, sylar::Stream::ptr stream);

		virtual std::string statusString();

		const std::string& getName() const { return m_name; }
		const std::string& getVersion() const { return m_version; }
		const std::string& getFilename() const { return m_filename; }
		const std::string& getId() const { return m_id; }

		void setFilename(const std::string& v) { m_filename = v; }

		uint32_t getType() const { return m_type; }

		////注册和查询服务的方法
		//void registerService(const std::string& server_type,
		//	const std::string& domain, const std::string& service);
		//void addRegisterParam(const std::string& key, const std::string& val);
		//void queryService(const std::string& domain, const std::string& service);

		////获取服务的IP和端口
		//static std::string GetServiceIPPort(const std::string& server_type);

	protected:
		//获取服务的IP和端口
		//std::string getServiceIPPort(const std::string& server_type);

	protected:
		//名称
		std::string m_name;
		//版本
		std::string m_version;
		//文件名
		std::string m_filename;
		//id
		std::string m_id;
		//类型
		uint32_t m_type;

	};


	class RockModule : public Module {
	public:
		typedef std::shared_ptr<RockModule> ptr;
		RockModule(const std::string& name
			, const std::string& version
			, const std::string& filename);

		virtual bool handleRockRequest(sylar::RockRequest::ptr request
			, sylar::RockResponse::ptr response
			, sylar::RockStream::ptr stream) = 0;

		virtual bool handleRockNotify(sylar::RockNotify::ptr notify
			, sylar::RockStream::ptr stream) = 0;

		virtual bool handleRequest(sylar::Message::ptr req
			, sylar::Message::ptr rsp
			, sylar::Stream::ptr stream);

		virtual bool handleNotify(sylar::Message::ptr notify
			, sylar::Stream::ptr stream);

	};


	class ModuleManager {
	public:
		using RWMutexType = RWMutex;

		ModuleManager() {}

		void add(Module::ptr m);
		void del(const std::string& name);
		void delAll();

		void init();

		Module::ptr get(const std::string& name);

		void onConnect(Stream::ptr stream);
		void onDisconnect(Stream::ptr stream);

		void listAll(std::vector<Module::ptr>& ms);
		void listByType(uint32_t type, std::vector<Module::ptr>& ms);
		void foreach(uint32_t type, std::function<void(Module::ptr)> cb);

	private:
		void initModule(const std::string& path);

	private:
		RWMutexType m_mutex;
		std::unordered_map<std::string, Module::ptr> m_modules;
		std::unordered_map<uint32_t, std::unordered_map<std::string, Module::ptr> > m_type2Modules;
	};

	using ModuleMgr = Singleton<ModuleManager>;



}
#endif