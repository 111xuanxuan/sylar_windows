#pragma once
#ifndef __SYLAR_ENDIAN_H__
#define __SYLAR_ENDIAN_H__

#include <type_traits>
#include <stdint.h>
#include <stdlib.h>

#define LITTLE_ENDIAN 1
#define BIG_ENDIAN 2


#if '\x01\x02\x03\x04'==0x01020304
#define BYTE_ORDER BIG_ENDIAN
#elif '\x01\x02\x03\x04'==0x04030201
#define BYTE_ORDER LITTLE_ENDIAN
#else
#error "unknown endian"
#endif 

namespace sylar {


	//8字节类型的字节序转换
	template<typename T>
	typename std::enable_if_t<sizeof(T) == sizeof(uint64_t), T> byteswap(T value) {
		return  static_cast<T>(_byteswap_uint64((unsigned long long)value));
	}

	//4字节类型的字节序转换
	template<typename T>
	typename std::enable_if_t<sizeof(T) == sizeof(uint32_t), T> byteswap(T value) {
		return static_cast<T>(_byteswap_ulong((unsigned long)value));
	}

	//2字节类型的字节序转换
	template<typename T>
	typename std::enable_if_t<sizeof(T) == sizeof(uint16_t), T> byteswap(T value) {
		return static_cast<T>(_byteswap_ushort((unsigned short)value));
	}


#if BYTE_ORDER == BIG_ENDIAN

	template<typename T>
	T byteswapOnLittleEndian(T t) {
		return t;
	}

	template<typename T>
	T byteswapOnBigEndian(T t) {
		return byteswap(t);
	}

#else

	template<typename T>
	T byteswapOnLittleEndian(T t) {
		return byteswap(t);
	}

	template<typename T>
	T byteswapOnBigEndian(T t) {
		return t;
	}

#endif // BYTE_ORDER == BIG_ENDIAN



}
#endif