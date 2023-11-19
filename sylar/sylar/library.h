#pragma once
#ifndef __SYLAR_LIBRARY_H__
#define __SYLAR_LIBRARY_H__


#include <memory>
#include "module.h"

namespace sylar {


	class Library {
	public:
		//加载指定模块
		static Module::ptr GetModule(const std::string& path);
	};

}

#endif