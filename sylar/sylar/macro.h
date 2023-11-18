#pragma once
#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <assert.h>
#include <string>
#include "log.h"


#if defined __GNUC__ || defined __llvm__
/// LIKCLY ��ķ�װ, ���߱������Ż�,��������ʳ���
#   define SYLAR_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY ��ķ�װ, ���߱������Ż�,��������ʲ�����
#   define SYLAR_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)      (x)
#endif



///���Ժ��װ
#define SYLAR_ASSERT(x) \
	if(SYLAR_UNLIKELY(!(x))){\
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
	}

/// ���Ժ��װ
#define SYLAR_ASSERT2(x, w) \
    if(SYLAR_UNLIKELY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }


#define _SYLAR_STR(...) #__VA_ARGS__
#define SYLAR_TSTR(...) _SYLAR_STR(__VA_ARGS__)
#define SYLAR_DSTR(s, d) (NULL != (s) ? (const char*)(s) : (const char*)(d))
#define SYLAR_SSTR(s) SYLAR_DSTR(s, "")
#define SYLAR_SLEN(s) (NULL != (s) ? strlen((const char*)(s)) : 0)


#endif