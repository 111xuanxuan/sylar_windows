#include "library.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "env.h"



namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	using create_module = Module * (*)();
	using destory_module = void(*)(Module*);

	//用于模块的销毁
	class ModuleCloser {
	public:
		ModuleCloser(HMODULE handle, destory_module d)
			:m_handle(handle)
			, m_destory(d) {
		}
		
		void operator()(Module* module) {
			std::string name = module->getName();
			std::string version = module->getVersion();
			std::string path = module->getFilename();
			//调用回调
			m_destory(module);
			//释放链接库
			BOOL rt = FreeLibrary(m_handle);
			if (!rt) {
				SYLAR_LOG_ERROR(g_logger) << "dlclose handle fail handle="
					<< m_handle << " name=" << name
					<< " version=" << version
					<< " path=" << path
					<< " error=" << GetLastError();
			}
			else {
				SYLAR_LOG_INFO(g_logger) << "destory module=" << name
					<< " version=" << version
					<< " path=" << path
					<< " handle=" << m_handle
					<< " success";
			}
		}
	private:
		//动态链接库句柄
		HMODULE m_handle;
		//释放链接库的回调函数
		destory_module m_destory;
	};


	sylar::Module::ptr Library::GetModule(const std::string& path)
	{
		HMODULE handle = LoadLibraryA(path.c_str());
		if (!handle) {
			SYLAR_LOG_ERROR(g_logger) << "cannot load library path="
				<< path << " error=" << GetLastError();
			return nullptr;
		}

		create_module create = (create_module)GetProcAddress(handle, "CreateModule");
		if (!creat) {
			SYLAR_LOG_ERROR(g_logger) << "cannot load symbol CreateModule in "
				<< path << " error=" << GetLastError();
			FreeLibrary(handle);
			return nullptr;
		}

		destory_module destory = (destory_module)GetProcAddress(handle, "DestoryModule");
		if (!destory) {
			SYLAR_LOG_ERROR(g_logger) << "cannot load symbol DestoryModule in "
				<< path << " error=" << GetLastError();
			FreeLibrary(handle);
			return nullptr;
		}

		Module::ptr module(create(), ModuleCloser(handle, destory));
		module->setFilename(path);
		SYLAR_LOG_INFO(g_logger) << "load module name=" << module->getName()
			<< " version=" << module->getVersion()
			<< " path=" << module->getFilename()
			<< " success";
		Config::LoadFromConfDir(EnvMgr::GetInstance()->getConfigPath(), true);
		return module;
	}

}