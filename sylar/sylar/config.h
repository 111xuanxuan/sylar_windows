#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <boost/lexical_cast.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <shared_mutex>
#include "mutex.h"
#include "log.h"

#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

namespace sylar {


	/**
	* @brief 配置变量的基类
	*/
	class ConfigVarBase {
	public:
		using ptr = std::shared_ptr<ConfigVarBase>;

		ConfigVarBase(const std::string& name, const std::string& description = "")
			:m_name{ name }, m_description{ description }{
			std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
		}

		virtual ~ConfigVarBase() {}

		/**
		* @brief 返回配置参数名称
		*/
		const std::string& getName() const { return m_name; }

		/**
		 * @brief 返回配置参数的描述
		 */
		const std::string& getDescription() const { return m_description; }

		/**
		 * @brief 转成字符串
		 */
		virtual std::string toString() = 0;

		/**
		 * @brief 从字符串初始化值
		 */
		virtual bool fromString(const std::string& val) = 0;

		/**
		 * @brief 返回配置参数值的类型名称
		 */
		virtual std::string getTypeName() const = 0;

	protected:

		/// 配置参数的名称
		std::string m_name;
		/// 配置参数的描述
		std::string m_description;
	};

	/**
	 * 类型转换模板(F 源类型，T 目标类型)
	 */
	template<typename F, typename T>
	class LexicalCast {
	public:

		/**
		* @brief 类型转换
		* @param[in] v 源类型值
		* @return 返回v转换后的目标类型
		* @exception 当类型不可转换时抛出异常
		*/
		T operator()(const F& v) {
			return boost::lexical_cast<T>(v);
		}
	};

	template<typename T>
	class LexicalCast<std::string, std::vector<T>> {
	public:

		std::vector<T> operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			typename std::vector<T> vec;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); ++i)
			{
				ss.str("");
				ss << node[i];
				vec.push_back(LexicalCast<std::string, T>()(ss.str()));
			}
			return vec;
		}
	};

	template<typename T>
	class LexicalCast<std::vector<T>, std::string> {
	public:
		std::string operator()(const std::vector<T>& v) {
			YAML::Node node(YAML::NodeType::Sequence);
			for (auto& i : v)
			{
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	template<class T>
	class LexicalCast<std::string, std::list<T> > {
	public:
		std::list<T> operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			typename std::list<T> vec;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); ++i) {
				ss.str("");
				ss << node[i];
				vec.push_back(LexicalCast<std::string, T>()(ss.str()));
			}
			return vec;
		}
	};

	template<class T>
	class LexicalCast<std::list<T>, std::string> {
	public:
		std::string operator()(const std::list<T>& v) {
			YAML::Node node(YAML::NodeType::Sequence);
			for (auto& i : v) {
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	template<class T>
	class LexicalCast<std::string, std::set<T> > {
	public:
		std::set<T> operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			typename std::set<T> vec;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); ++i) {
				ss.str("");
				ss << node[i];
				vec.insert(LexicalCast<std::string, T>()(ss.str()));
			}
			return vec;
		}
	};

	template<class T>
	class LexicalCast<std::set<T>, std::string> {
	public:
		std::string operator()(const std::set<T>& v) {
			YAML::Node node(YAML::NodeType::Sequence);
			for (auto& i : v) {
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	template<class T>
	class LexicalCast<std::string, std::unordered_set<T> > {
	public:
		std::unordered_set<T> operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			typename std::unordered_set<T> vec;
			std::stringstream ss;
			for (size_t i = 0; i < node.size(); ++i) {
				ss.str("");
				ss << node[i];
				vec.insert(LexicalCast<std::string, T>()(ss.str()));
			}
			return vec;
		}
	};

	template<class T>
	class LexicalCast<std::unordered_set<T>, std::string> {
	public:
		std::string operator()(const std::unordered_set<T>& v) {
			YAML::Node node(YAML::NodeType::Sequence);
			for (auto& i : v) {
				node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	template<typename T>
	class LexicalCast<std::string, std::map<std::string, T>> {
	public:
		std::map<std::string, T> operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			typename std::map<std::string, T> vec;
			std::stringstream ss;
			for (auto it = node.begin(); it != node.end(); ++it)
			{
				ss.str("");
				ss << it->second;
				vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
			}
			return vec;
		}
	};

	template<class T>
	class LexicalCast<std::map<std::string, T>, std::string> {
	public:
		std::string operator()(const std::map<std::string, T>& v) {
			YAML::Node node(YAML::NodeType::Map);
			for (auto& i : v) {
				node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	template<class T>
	class LexicalCast<std::string, std::unordered_map<std::string, T> > {
	public:
		std::unordered_map<std::string, T> operator()(const std::string& v) {
			YAML::Node node = YAML::Load(v);
			typename std::unordered_map<std::string, T> vec;
			std::stringstream ss;
			for (auto it = node.begin();
				it != node.end(); ++it) {
				ss.str("");
				ss << it->second;
				vec.insert(std::make_pair(it->first.Scalar(),
					LexicalCast<std::string, T>()(ss.str())));
			}
			return vec;
		}
	};

	template<class T>
	class LexicalCast<std::unordered_map<std::string, T>, std::string> {
	public:
		std::string operator()(const std::unordered_map<std::string, T>& v) {
			YAML::Node node(YAML::NodeType::Map);
			for (auto& i : v) {
				node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
			}
			std::stringstream ss;
			ss << node;
			return ss.str();
		}
	};

	/**
	* @brief 配置参数模板子类,保存对应类型的参数值
	* @details T 参数的具体类型
	*          FromStr 从std::string转换成T类型的仿函数
	*          ToStr 从T转换成std::string的仿函数
	*          std::string 为YAML格式的字符串
	*/
	template<typename T, typename FromStr = LexicalCast<std::string, T>
		, typename ToStr = LexicalCast<T, std::string>>
			class ConfigVar :public ConfigVarBase {
			public:
				using ptr = std::shared_ptr<ConfigVar>;
				using on_change_cb = std::function<void(const T& old_value, const T& new_value)>;

				ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
					:ConfigVarBase(name, description)
					, m_value(default_value) {
				}

				std::string toString()override {
					try
					{
						ReadLock lock{ m_mutex };
						return ToStr()(m_value);
					}
					catch (const std::exception& e)
					{
					}
					return "";
				}

				bool fromString(const std::string& val)override {
					try
					{
						setValue(FromStr()(val));
						return true;
					}
					catch (const std::exception&)
					{
					}
					return false;
				}

				const T getValue() {
					ReadLock lock{ m_mutex };
					return m_value;
				}

				void setValue(const T& v) {

					{
						ReadLock lock{ m_mutex };
						if (v == m_value) {
							return;
						}
						for (auto& i : m_cbs)
						{
							i.second(m_value, v);
						}
					}

					WriteLock lock{ m_mutex };
					m_value = v;
				}

				/**
				* @brief 返回参数值的类型名称(typeinfo)
				*/
				std::string getTypeName() const override { return TypeToName<T>(); }

				/**
				* @brief 添加变化回调函数
				* @return 返回该回调函数对应的唯一id,用于删除回调
				*/
				uint64_t addListener(on_change_cb cb) {
					static uint64_t s_fun_id = 0;
					WriteLock lock(m_mutex);
					m_cbs[++s_fun_id] = cb;
					return s_fun_id;
				}

				/**
				 * @brief 删除回调函数
				* @param[in] key 回调函数的唯一id
				 */
				void delListener(uint64_t key) {
					WriteLock lock(m_mutex);
					m_cbs.erase(key);
				}

				/**
				* @brief 获取回调函数
				* @param[in] key 回调函数的唯一id
				* @return 如果存在返回对应的回调函数,否则返回nullptr
				*/
				on_change_cb getListener(uint64_t key) {
					ReadLock lock(m_mutex);
					auto it = m_cbs.find(key);
					return it == m_cbs.end() ? nullptr : it->second;
				}

				/**
				* @brief 清理所有的回调函数
				*/
				void clearListener() {
					WriteLock lock(m_mutex);
					m_cbs.clear();
				}

			private:
				RWMutex m_mutex;
				T m_value;
				std::map<uint64_t, on_change_cb> m_cbs;
		};

		/**
		* @brief ConfigVar的管理类
		* @details 提供便捷的方法创建/访问ConfigVar
		*/
		class Config {
		public:
			using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;

			/**
			* @brief 获取/创建对应参数名的配置参数
			* @param[in] name 配置参数名称
			* @param[in] default_value 参数默认值
			* @param[in] description 参数描述
			* @details 获取参数名为name的配置参数,如果存在直接返回
			*          如果不存在,创建参数配置并用default_value赋值
			* @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
			* @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
			*/

			template<typename T,typename...Args>
			static typename ConfigVar<T,Args...>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "") {
				WriteLock lock{ s_mutex };
				auto it = s_datas.find(name);

				if (it != s_datas.end()) {

					auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);

					if (tmp) {
						SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
						return tmp;
					}
					else {
						SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
							<< TypeToName<T>() << " real_type=" << it->second->getTypeName()
							<< " " << it->second->toString();
						return nullptr;
					}

				}

				if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
					SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
					throw std::invalid_argument(name);
				}

				typename ConfigVar<T,Args...>::ptr v(new ConfigVar<T>(name, default_value, description));

				s_datas[name] = v;
				return v;
			}

			/**
			 * 查找配置参数
			 */
			template<typename T,typename ...Args>
			static typename ConfigVar<T,Args...>::ptr Lookup(const std::string& name) {
				ReadLock lock{ s_mutex };
				auto it = s_datas.find(name);
				if (it == s_datas.end()) {
					return nullptr;
				}
				return  std::dynamic_pointer_cast<ConfigVar<T,Args...>>(it->second);
			}

			/**
			* @brief 使用YAML::Node初始化配置模块
			*/
			static void LoadFromYaml(const YAML::Node& root);

			/**
			* @brief 加载path文件夹里面的配置文件
			*/
			static void LoadFromConfDir(const std::string& path, bool force = false);

			/**
			* @brief 查找配置参数,返回配置参数的基类
			* @param[in] name 配置参数名称
			*/
			static ConfigVarBase::ptr LookupBase(const std::string& name);

			/**
			* @brief 遍历配置模块里面所有配置项
			* @param[in] cb 配置项回调函数
			 */
			static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

		private:
			inline static RWMutex s_mutex;
			inline static ConfigVarMap s_datas;
		};
}

#endif