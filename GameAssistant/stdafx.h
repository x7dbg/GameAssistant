#pragma once
#ifndef STDAFX_H
#define STDAFX_H

#include "targetver.h"

// MFC头文件（MFC已包含windows.h，这里不需要再包含）
#include <afxwin.h>    // MFC核心头文件
#include <afxext.h>    // MFC扩展头文件
#include <afxcmn.h>    // MFC通用控件（CListCtrl和CComboBox等）
#include <afxdialogex.h> // MFC对话框扩展
#include <afxcontrolbars.h> // MFC功能区控件

// Windows API头文件（MFC已包含windows.h，这里不需要再包含）
#include <tlhelp32.h>   // 进程快照API
#include <process.h>   // 线程创建API（_beginthreadex等）

// C++标准库头文件（VS2015及更高版本可用）
#include <iostream>    // 输入输出流
#include <string>      // 字符串类
#include <vector>      // 容器
#include <map>         // 关联容器
#include <atomic>      // 原子操作（C++11）
#include <mutex>       // 互斥锁（C++11）
#include <algorithm>   // 算法
#include <ctime>       // 时间函数
#include <fstream>     // 文件流
#include <sstream>     // 字符串流

using namespace std;

// 调试版本内存检测
// MFC会自动定义DEBUG_NEW，此处仅为确保CRT调试分配生效
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
// 在MFC之后包含DEBUG_NEW，确保所有new调用被追踪
#define new DEBUG_NEW
#endif

// 注：此处不定义THREAD_WAIT_TIMEOUT和HEARTBEAT_INTERVAL
// 这些常量在GameAssistantComm.h中定义

#endif // STDAFX_H
