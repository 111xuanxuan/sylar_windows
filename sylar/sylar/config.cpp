#include "config.h"
#include "log.h"
#include "util.h"
#include "env.h"

namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	ConfigVarBase::ptr Config::LookupBase(const std::string& name)
	{
		ReadLock lock{ Config::s_mutex };
		auto it = s_datas.find(name);
		return it == s_datas.end() ? nullptr : it->second;
	}

	static void ListAllMember(const std::string& prefix, const YAML::Node& node, std::list<std::pair<std::string, const YAML::Node> >& output) {
		if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
			SYLAR_LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " : " << node;
			return;
		}

		output.push_back(std::make_pair(prefix, node));
		if (node.IsMap()) {
			for (auto it = node.begin(); it != node.end(); ++it)
			{
				ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
			}
		}
	}

	void Config::LoadFromYaml(const YAML::Node& root)
	{
		std::list<std::pair<std::string, const YAML::Node>> all_nodes;
		ListAllMember("", root, all_nodes);
		for (auto& i : all_nodes)
		{
			std::string key = i.first;
			if (key.empty()) {
				continue;
			}

			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			ConfigVarBase::ptr var = LookupBase(key);

			if (var) {
				if (i.second.IsScalar()) {
					var->fromString(i.second.Scalar());
				}
				else {
					std::stringstream ss;
					ss << i.second;
					var->fromString(ss.str());
				}
			}
		}
	}

	static std::map<std::string, uint64_t> s_file2modifytime;
	static std::mutex s_mutex;

	void Config::LoadFromConfDir(const std::string& path, bool force /*= false*/)
	{
		std::string absoulte_path = sylar::EnvMgr::GetInstance()->getAbsolutePath(path);
		std::vector<std::string> files;
		FSUtil::ListAllFile(files, absoulte_path, ".yml");

		for (auto& i : files)
		{
			{
				struct stat st;
				_stat(i.c_str(), (struct _stat64i32*)&st);
				std::lock_guard lock{ s_mutex };
				if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
					continue;
				}
				s_file2modifytime[i] = st.st_mtime;
			}

			try
			{
				YAML::Node root = YAML::LoadFile(i);
				LoadFromYaml(root);
				SYLAR_LOG_INFO(g_logger) << "LoadConfFile file="
					<< i << " ok"<<std::endl;
			}
			catch (...) {
				SYLAR_LOG_ERROR(g_logger) << "LoadConfFile file="
					<< i << " failed" << std::endl;
			}
		}
	}

	void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
	{
		ReadLock lock{ Config::s_mutex };
		ConfigVarMap& m = Config::s_datas;
		for (auto it = m.begin(); it != m.end(); ++it)
		{
			cb(it->second);
		}
	}
}