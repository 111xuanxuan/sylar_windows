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
	* @brief ���ñ����Ļ���
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
		* @brief �������ò�������
		*/
		const std::string& getName() const { return m_name; }

		/**
		 * @brief �������ò���������
		 */
		const std::string& getDescription() const { return m_description; }

		/**
		 * @brief ת���ַ���
		 */
		virtual std::string toString() = 0;

		/**
		 * @brief ���ַ�����ʼ��ֵ
		 */
		virtual bool fromString(const std::string& val) = 0;

		/**
		 * @brief �������ò���ֵ����������
		 */
		virtual std::string getTypeName() const = 0;

	protected:

		/// ���ò���������
		std::string m_name;
		/// ���ò���������
		std::string m_description;
	};

	/**
	 * ����ת��ģ��(F Դ���ͣ�T Ŀ������)
	 */
	template<typename F, typename T>
	class LexicalCast {
	public:

		/**
		* @brief ����ת��
		* @param[in] v Դ����ֵ
		* @return ����vת�����Ŀ������
		* @exception �����Ͳ���ת��ʱ�׳��쳣
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
	* @brief ���ò���ģ������,�����Ӧ���͵Ĳ���ֵ
	* @details T �����ľ�������
	*          FromStr ��std::stringת����T���͵ķº���
	*          ToStr ��Tת����std::string�ķº���
	*          std::string ΪYAML��ʽ���ַ���
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
				* @brief ���ز���ֵ����������(typeinfo)
				*/
				std::string getTypeName() const override { return TypeToName<T>(); }

				/**
				* @brief ��ӱ仯�ص�����
				* @return ���ظûص�������Ӧ��Ψһid,����ɾ���ص�
				*/
				uint64_t addListener(on_change_cb cb) {
					static uint64_t s_fun_id = 0;
					WriteLock lock(m_mutex);
					m_cbs[++s_fun_id] = cb;
					return s_fun_id;
				}

				/**
				 * @brief ɾ���ص�����
				* @param[in] key �ص�������Ψһid
				 */
				void delListener(uint64_t key) {
					WriteLock lock(m_mutex);
					m_cbs.erase(key);
				}

				/**
				* @brief ��ȡ�ص�����
				* @param[in] key �ص�������Ψһid
				* @return ������ڷ��ض�Ӧ�Ļص�����,���򷵻�nullptr
				*/
				on_change_cb getListener(uint64_t key) {
					ReadLock lock(m_mutex);
					auto it = m_cbs.find(key);
					return it == m_cbs.end() ? nullptr : it->second;
				}

				/**
				* @brief �������еĻص�����
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
		* @brief ConfigVar�Ĺ�����
		* @details �ṩ��ݵķ�������/����ConfigVar
		*/
		class Config {
		public:
			using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;

			/**
			* @brief ��ȡ/������Ӧ�����������ò���
			* @param[in] name ���ò�������
			* @param[in] default_value ����Ĭ��ֵ
			* @param[in] description ��������
			* @details ��ȡ������Ϊname�����ò���,�������ֱ�ӷ���
			*          ���������,�����������ò���default_value��ֵ
			* @return ���ض�Ӧ�����ò���,������������ڵ������Ͳ�ƥ���򷵻�nullptr
			* @exception ��������������Ƿ��ַ�[^0-9a-z_.] �׳��쳣 std::invalid_argument
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
			 * �������ò���
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
			* @brief ʹ��YAML::Node��ʼ������ģ��
			*/
			static void LoadFromYaml(const YAML::Node& root);

			/**
			* @brief ����path�ļ�������������ļ�
			*/
			static void LoadFromConfDir(const std::string& path, bool force = false);

			/**
			* @brief �������ò���,�������ò����Ļ���
			* @param[in] name ���ò�������
			*/
			static ConfigVarBase::ptr LookupBase(const std::string& name);

			/**
			* @brief ��������ģ����������������
			* @param[in] cb ������ص�����
			 */
			static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

		private:
			inline static RWMutex s_mutex;
			inline static ConfigVarMap s_datas;
		};
}

#endif