#include "env.h"
#include "log.h"
#include "util.h"
#include "config.h"


namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


	bool Env::init(int argc, char** argv)
	{
		char path[1024] = { 0 };
#ifdef _WIN32
		GetModuleFileNameA(NULL, path,1024);
		if (strlen(path) == 0) {
			m_exe = argv[0];
		}
#else
		char link[1024] = { 0 };
		sprintf(link, "/proc/%d/exe", getpid());
		readlink(link, path, sizeof(path));
#endif // _WIN32

		m_exe = path;

		auto pos = m_exe.find_last_of(PATH_SEPARATOR);
		m_cwd = m_exe.substr(0, pos) + PATH_SEPARATOR;

		m_program = argv[0];

		const char* now_key = nullptr;

		for (int i=1;i<argc;++i)
		{
			if (argv[i][0] == '-') {
				if (strlen(argv[i]) > 1) {
					if (now_key) {
						add(now_key, "");
					}
					now_key = argv[i] + 1;
				}
				else {
					SYLAR_LOG_ERROR(g_logger) << "invalid arg idx=" << i
						<< " val=" << argv[i];
					return false;
				}
			}
			else {
				if (now_key) {
					add(now_key, argv[i]);
					now_key = nullptr;
				}
				else {
					SYLAR_LOG_ERROR(g_logger) << "invalid arg idx=" << i
						<< " val=" << argv[i];
					return false;
				}
			}
		}

		if (now_key) {
			add(now_key, "");
		}
		return true;

	}

	void Env::add(const std::string& key, const std::string& val)
	{
		WriteLock lock{ m_mutex };
		m_args[key] = val;
	}

	bool Env::has(const std::string& key)
	{
		ReadLock lock{ m_mutex };
		auto it = m_args.find(key);
		return it != m_args.end();
	}

	void Env::del(const std::string& key)
	{
		WriteLock lock{ m_mutex };
		m_args.erase(key);
	}

	std::string Env::get(const std::string& key, const std::string& default_value /*= ""*/)
	{
		ReadLock lock(m_mutex);
		auto it = m_args.find(key);
		return it != m_args.end() ? it->second : default_value;
	}

	void Env::addHelp(const std::string& key, const std::string& desc)
	{
		removeHelp(key);
		WriteLock lock{ m_mutex };
		m_helps.push_back(std::make_pair(key, desc));
	}

	void Env::removeHelp(const std::string& key)
	{
		WriteLock lock{ m_mutex };
		for (auto it = m_helps.begin(); it != m_helps.end();) {
			if (it->first == key) {
				it = m_helps.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void Env::printHelp()
	{
		ReadLock lock{ m_mutex };
		std::cout << "Usage: " << m_program << " [options]" << std::endl;
		for (auto& i : m_helps) {
			std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
		}
	}



	bool Env::setEnv(const std::string& key, const std::string& val)
	{
#ifdef _WIN32
		std::string tmp = key + '=' + val;
		return _putenv(tmp.c_str()) == 0;
#else 
		return !setenv(key.c_str(), val.c_str(), 1);
#endif // _WIN32

	}

	std::string Env::getEnv(const std::string& key, const std::string& default_value /*= ""*/)
	{
		const char* v = getenv(key.c_str());
		if (v==nullptr)
		{
			return default_value;
		}
		return v;
	}

	std::string Env::getAbsolutePath(const std::string& path) const
	{
		if (path.empty()) {
			return PATH_SEPARATOR_STRING;
		}

		if (path[0] == PATH_SEPARATOR) {
			return path;
		}

		return m_cwd + path;
	}

	std::string Env::getAbsoluteWorkPath(const std::string& path) const
	{
		if (path.empty()) {
			return PATH_SEPARATOR_STRING;
		}

		if (path[0] == PATH_SEPARATOR) {
			return path;
		}

		static sylar::ConfigVar<std::string>::ptr g_server_work_path= sylar::Config::Lookup<std::string>("server.work_path");
		return g_server_work_path->getValue() + PATH_SEPARATOR_STRING + path;
	}

	std::string Env::getConfigPath()
	{
		return getAbsolutePath(get("c", "conf"));
	}

}