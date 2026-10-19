#pragma once
#ifndef STDAFX_H
#define STDAFX_H

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

// Windows 头文件
#include <windows.h>
#include <tlhelp32.h>
#include <process.h>
#include <tchar.h>

// C++ 标准库
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>

// Lua 头文件
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using namespace std;

#endif // STDAFX_H