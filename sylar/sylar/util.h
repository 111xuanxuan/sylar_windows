#pragma once
#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <cstdint>
#include <string>
#include <time.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <time.h>
#include <algorithm>
#include <io.h>
#include <direct.h>
#include <filesystem>
#include <format>
#include <openssl/md5.h>

#ifdef _WIN32

#define PATH_SEPARATOR	'\\'
#define PATH_SEPARATOR_STRING "\\"

#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include <WS2tcpip.h>

#include <windows.h>


#else
#define PATH_SEPARATOR	'/'
#define PATH_SEPARATOR_STRING "/"
#endif

namespace sylar {

	uint32_t GetThreadId();
	uint64_t GetFiberId();

	//获取当前的调用栈
	void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
	//获取当前栈信息的字符串
	std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

	uint64_t GetCurrentMS();
	uint64_t GetCurrentUS();

	std::string ToUpper(const std::string& name);
	std::string ToLower(const std::string& name);

	std::string Time2Str(std::time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()), const std::string& format = "%Y-%m-%d %H:%M:%S");
	std::time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

	std::string md5sum(const std::vector<WSABUF>& buffers);

	class FSUtil {
	public:

		static void ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix);

		static bool Mkdir(const std::string& dirname);

		static bool OpenForRead(std::ifstream& ifs, const std::string& filename
			, std::ios_base::openmode mode);

		static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
			, std::ios_base::openmode mode);

		static bool Unlink(const std::string& filename, bool exist = false);

		static std::string Dirname(const std::string& filename);

		static std::string Basename(const std::string& filename);
	};

	template<typename T>
	const char* TypeToName() {
		static const char* s_name = typeid(T).name();
		return s_name;
	}

	template<typename T,typename...Args>
	inline std::shared_ptr<T> protected_make_shared(Args&&...args) {
		struct Helper :T {
			Helper(Args&&...args) :T{args...} {
			}
		};
		return std::make_shared<Helper>(std::forward<Args>(args)...);
	}

}
#endif