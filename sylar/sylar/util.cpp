#include "util.h"
#include "fiber.h"
#include "log.h"

#ifdef _WIN32
#include <dbghelp.h>
#pragma comment(lib,"dbghelp.lib")
#endif
#include <random>

namespace  fs = std::filesystem;

namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	uint32_t GetThreadId()
	{
#ifdef _WIN32

#endif // _WIN32
		return GetCurrentThreadId();
	}

	uint64_t GetFiberId()
	{
		return sylar::Fiber::GetFiberId();
	}


	void Backtrace(std::vector<std::string>& bt, int size /*= 64*/, int skip /*= 1*/)
	{
		void** array = (void**)malloc(sizeof(void*)*size);

#ifdef _WIN32
		HANDLE process;
		SYMBOL_INFO* symbol;
		process = GetCurrentProcess();
		if (!SymInitialize(process, nullptr, TRUE)) {
			SYLAR_LOG_ERROR(g_logger) << "initialize synbols error";
			return;
		}
		size_t s= CaptureStackBackTrace(skip, size, array, nullptr);
		symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char),1);
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		for (int i=0;i<s;++i)
		{
			SymFromAddr(process, (DWORD64)(array[i]), 0, symbol);
			bt.push_back(symbol->Name);
		}

		SymCleanup(process);
		free(symbol);
#else
		size_t s= ::backtrace(array, size);
		char** strings = backtrace_symbols(array, s);
		if (strings == NULL) {
			SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
			return;
		}

		for (size_t i = skip; i < s; ++i) {
			bt.push_back(demangle(strings[i]));
		}

		free(strings);
#endif // _WIN32

		free(array);
	}

	std::string BacktraceToString(int size /*= 64*/, int skip /*= 2*/, const std::string& prefix /*= ""*/)
	{
		std::vector<std::string> bt;
		Backtrace(bt, size, skip);
		std::stringstream ss;
		for (size_t i=0;i<bt.size();++i)
		{
			ss << prefix << bt[i] << std::endl;
		}
		return ss.str();
	}

	uint64_t GetCurrentMS()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	uint64_t GetCurrentUS()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	std::string ToUpper(const std::string& name)
	{
		std::string rt = name;
		std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
		return rt;
	}

	std::string ToLower(const std::string& name)
	{
		std::string rt = name;
		std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
		return rt;
	}

	std::string Time2Str(std::time_t ts, const std::string& format)
	{
		static char buf[64];
		struct tm t {};
		localtime_s(&t, &ts);
		std::strftime(buf, sizeof(buf), format.c_str(), &t);
		return buf;
	}

	std::time_t Str2Time(const char* str, const char* format)
	{
		std::istringstream iss(str);
		std::tm t = {};
		iss >> std::get_time(&t, format);
		return mktime(&t);
	}

	std::string md5sum(const std::vector<WSABUF>& buffers)
	{
		MD5_CTX ctx;
		unsigned char digest[MD5_DIGEST_LENGTH];
		char mdString[33];

		MD5_Init(&ctx);

		for (const auto& buf : buffers) {
			if (buf.buf && buf.len > 0) {
				MD5_Update(&ctx, buf.buf, buf.len);
			}
		}

		MD5_Final(digest, &ctx);

		for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
			sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);

		return std::string(mdString);
	}


	static int __lstat(const char* file, struct stat* st = nullptr) {
		struct stat lst;
		int ret = _stat(file, (struct _stat64i32*)&lst);
		if (st) {
			*st = lst;
		}
		return ret;
	}

	static int __mkdir(const char* dirname) {
		if (_access(dirname, 0) == 0) {
			return 0;
		}
		return  _mkdir(dirname);
	}

	void FSUtil::ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix)
	{
		if (_access(path.c_str(), 0) != 0) {
			return;
		}

		fs::path p{ path };

		if (fs::is_empty(p))
			return;

		auto dir = fs::directory_iterator{ p };

		for (auto& it : dir)
		{
			auto& path = it.path();

			if (fs::is_directory(path)) {
				ListAllFile(files, path.string(), subfix);
			}
			else if (subfix.empty() || path.extension().string() == subfix) {
				files.push_back(path.string());
			}
		}
	}

	bool FSUtil::Mkdir(const std::string& dirname)
	{
		if (__lstat(dirname.c_str()) == 0) {
			return true;
		}

		char* path = _strdup(dirname.c_str());
		char* ptr = strchr(path + 1, PATH_SEPARATOR);
		do
		{
			for (; ptr; *ptr = PATH_SEPARATOR, ptr = strchr(ptr + 1, PATH_SEPARATOR))
			{
				*ptr = '\0';
				if (__mkdir(path) != 0) {
					break;
				}
			}
			if (ptr != nullptr) {
				break;
			}
			else if (__mkdir(path) != 0) {
				break;
			}

			free(path);
			return true;
		} while (0);
		free(path);
		return false;
	}

	bool FSUtil::OpenForRead(std::ifstream& ifs, const std::string& filename, std::ios_base::openmode mode)
	{
		ifs.open(filename.c_str(), mode);
		return ifs.is_open();
	}

	bool FSUtil::OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode)
	{
		ofs.open(filename.c_str(), mode);
		if (!ofs.is_open()) {
#ifdef _WIN32
			fs::path p{ filename };
			try
			{
				fs::create_directories(p.parent_path());
			}
			catch (const fs::filesystem_error& e)
			{
				std::cout << std::format("create directory error ,path: {}, code: {},info: {}.\n", e.path1().string(), e.code().value(), e.what());
				return false;
			}
#else
			std::string dir = Dirname(filename);
			Mkdir(dir);
#endif // _WIN32

			ofs.open(filename.c_str(), mode);
			}
		}

	bool FSUtil::Unlink(const std::string& filename, bool exist)
	{
		if (!exist && __lstat(filename.c_str())) {
			return true;
		}
		return ::_unlink(filename.c_str()) == 0;
	}

	std::string FSUtil::Dirname(const std::string& filename)
	{
		if (filename.empty()) {
			return ".";
		}

		auto pos = filename.rfind(PATH_SEPARATOR);
		if (pos == 0) {
			return PATH_SEPARATOR_STRING;
		}
		else if (pos == std::string::npos) {
			return ".";
		}
		else {
			return filename.substr(0, pos);
		}
	}

	std::string FSUtil::Basename(const std::string& filename)
	{
		if (filename.empty()) {
			return filename;
		}

		auto pos = filename.rfind(PATH_SEPARATOR);
		if (pos == std::string::npos) {
			return filename;
		}
		else {
			return filename.substr(pos + 1);
		}
	}

	std::string random_string(size_t length)
	{
		static const char charset[] ="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		// 以随机值播种
		std::random_device r;

		// 选择 1 与 6 间的随机数
		std::default_random_engine e1(r());
		std::uniform_int_distribution<int> uniform_dist(0, sizeof(charset) - 1);

		auto rand_char = [&uniform_dist,&e1]()->char {
			return charset[uniform_dist(e1)];
		};
		std::string str(length, 0);
		std::generate_n(str.begin(), length, rand_char);
		return str;
	}

	}