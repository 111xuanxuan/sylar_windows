#include "module.h"
#include "config.h"
#include "env.h"
#include "library.h"

namespace sylar {

	static sylar::ConfigVar<std::string>::ptr g_module_path
		= Config::Lookup("module.path", std::string("module"), "module path");

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");



	Module::Module(const std::string& name, const std::string& version, const std::string& filename, uint32_t type /*= MODULE*/)
		:m_name(name)
		, m_version(version)
		, m_filename(filename)
		, m_id(name + "/" + version)
		, m_type(type)
	{

	}

	void Module::onBeforeArgsParse(int argc, char** argv)
	{

	}

	void Module::onAfterArgsParse(int argc, char** argv)
	{

	}

	bool Module::onLoad()
	{
		return true;
	}

	bool Module::onUnload()
	{
		return true;
	}

	bool Module::onConnect(sylar::Stream::ptr stream)
	{
		return true;
	}

	bool Module::onDisconnect(sylar::Stream::ptr stream)
	{
		return true;
	}

	bool Module::onServerReady()
	{
		return true;
	}

	bool Module::onServerUp()
	{
		return false;
	}

	bool Module::handleRequest(sylar::Message::ptr req, sylar::Message::ptr rsp, sylar::Stream::ptr stream)
	{
		SYLAR_LOG_DEBUG(g_logger) << "handleRequest req=" << req->toString()
			<< " rsp=" << rsp->toString() << " stream=" << stream;
		return true;
	}

	bool Module::handleNotify(sylar::Message::ptr notify, sylar::Stream::ptr stream)
	{
		SYLAR_LOG_DEBUG(g_logger) << "handleNotify nty=" << notify->toString()
			<< " stream=" << stream;
		return true;
	}

	std::string Module::statusString()
	{
		std::stringstream ss;
		ss << "Module name=" << getName()
			<< " version=" << getVersion()
			<< " filename=" << getFilename()
			<< std::endl;
		return ss.str();
	}

	RockModule::RockModule(const std::string& name, const std::string& version, const std::string& filename):Module(name,version,filename,ROCK)
	{

	}


	bool RockModule::handleRequest(sylar::Message::ptr req, sylar::Message::ptr rsp, sylar::Stream::ptr stream)
	{
		auto rock_req = std::dynamic_pointer_cast<RockRequest>(req);
		auto rock_rsp = std::dynamic_pointer_cast<RockResponse>(rsp);
		auto rock_stream = std::dynamic_pointer_cast<RockStream>(stream);
		return handleRockRequest(rock_req, rock_rsp, rock_stream);
	}

	bool RockModule::handleNotify(sylar::Message::ptr notify, sylar::Stream::ptr stream)
	{
		auto rock_nty = std::dynamic_pointer_cast<sylar::RockNotify>(notify);
		auto rock_stream = std::dynamic_pointer_cast<sylar::RockStream>(stream);
		return handleRockNotify(rock_nty, rock_stream);
	}

	void ModuleManager::add(Module::ptr m)
	{
		del(m->getId());
		WriteLock lock(m_mutex);
		m_modules[m->getId()] = m;
		m_type2Modules[m->getType()][m->getId()] = m;
	}

	void ModuleManager::del(const std::string& name)
	{
		Module::ptr module;
		WriteLock lock{ m_mutex };
		auto it = m_modules.find(name);
		if (it == m_modules.end()) {
			return;
		}
		module = it->second;
		m_modules.erase(it);
		m_type2Modules[module->getType()].erase(module->getId());
		if (m_type2Modules[module->getType()].empty()) {
			m_type2Modules.erase(module->getType());
		}
		lock.unlock();
		module->onUnload();

	}

	void ModuleManager::delAll()
	{
		ReadLock lock(m_mutex);
		auto tmp = m_modules;
		lock.unlock();

		for (auto& i : tmp) {
			del(i.first);
		}
	}

	void ModuleManager::init()
	{
		auto path=EnvMgr::GetInstance()->getAbsolutePath(g_module_path->getValue());

		std::vector<std::string> files;
		sylar::FSUtil::ListAllFile(files, path, ".so");

		std::sort(files.begin(), files.end());
		for (auto& i : files) {
			initModule(i);
		}

	}

	sylar::Module::ptr ModuleManager::get(const std::string& name)
	{
		ReadLock lock{ m_mutex };
		auto it = m_modules.find(name);
		return it == m_modules.end() ? nullptr : it->second;
	}

	void ModuleManager::foreach(uint32_t type, std::function<void(Module::ptr)> cb) {
		std::vector<Module::ptr> ms;
		listByType(type, ms);
		for (auto& i : ms) {
			cb(i);
		}
	}

	void ModuleManager::onConnect(Stream::ptr stream)
	{
		std::vector<Module::ptr> ms;
		listAll(ms);

		for (auto& m : ms) {
			m->onConnect(stream);
		}
	}

	void ModuleManager::onDisconnect(Stream::ptr stream)
	{
		std::vector<Module::ptr> ms;
		listAll(ms);

		for (auto& m : ms) {
			m->onDisconnect(stream);
		}
	}

	void ModuleManager::listAll(std::vector<Module::ptr>& ms)
	{
		ReadLock lock(m_mutex);
		for (auto& i : m_modules) {
			ms.push_back(i.second);
		}
	}

	void ModuleManager::listByType(uint32_t type, std::vector<Module::ptr>& ms)
	{
		ReadLock lock(m_mutex);
		auto it = m_type2Modules.find(type);
		if (it == m_type2Modules.end()) {
			return;
		}
		for (auto& i : it->second) {
			ms.push_back(i.second);
		}
	}

	void ModuleManager::initModule(const std::string& path)
	{
		Module::ptr m = Library::GetModule(path);
		if (m) {
			add(m);
		}
	}

}