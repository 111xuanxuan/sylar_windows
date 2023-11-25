#include "stream.h"
#include <algorithm>

namespace sylar {


	static constexpr int32_t g_socket_buff_size = 1024 * 16;


	int Stream::readFixSize(void* buffer, size_t length)
	{
		size_t offset = 0;
		int64_t left = length;
		static const int64_t MAX_LEN = g_socket_buff_size;
		while (left > 0) {
			int64_t len = read((char*)buffer + offset, min(left, MAX_LEN));
			if (len <= 0) {
				return len;
			}
			offset += len;
			left -= len;
		}
		return length;
	}

	int Stream::readFixSize(ByteArray::ptr ba, size_t length)
	{
		int64_t left = length;
		static const int64_t MAX_LEN = g_socket_buff_size;
		while (left > 0) {
			int64_t len = read(ba, min(left, MAX_LEN));
			if (len <= 0) {
				return len;
			}
			left -= len;
		}
		return length;
	}

	int Stream::writeFixSize(const void* buffer, size_t length)
	{
		size_t offset = 0;
		int64_t left = length;
		static const int64_t MAX_LEN = g_socket_buff_size ;
		while (left > 0) {
			int64_t len = write((const char*)buffer + offset, min(left, MAX_LEN));
			if (len <= 0) {
				return len;
			}
			offset += len;
			left -= len;
		}
		return length;
	}

	int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
		int64_t left = length;
		while (left > 0) {
			static const int64_t MAX_LEN = g_socket_buff_size;
			int64_t len = write(ba, min(left, MAX_LEN));
			if (len <= 0) {
				return len;
			}
			left -= len;
		}
		return length;
	}

}