#pragma once
#ifndef GAME_ASSISTANT_DLL_H
#define GAME_ASSISTANT_DLL_H

#ifdef GAMEASSISTANTDLL_EXPORTS
#define GAMEASSISTANTDLL_API extern "C" __declspec(dllexport)
#else
#define GAMEASSISTANTDLL_API extern "C" __declspec(dllimport)
#endif

#include "stdafx.h"
#include "GameAssistantComm.h"

// DLL 导出函数声明

// 初始化客户端
extern "C" __declspec(dllexport) BOOL InitClient();
// 反初始化客户端
extern "C" __declspec(dllexport) void UninitClient();
// 检查客户端连接状态
extern "C" __declspec(dllexport) BOOL IsClientConnected();
// 发送测试数据
extern "C" __declspec(dllexport) BOOL SendTestData();

// 执行Lua脚本
GAMEASSISTANTDLL_API BOOL ExecuteLuaScript(const char* scriptName, const char* scriptContent);

// 暂停Lua脚本
GAMEASSISTANTDLL_API void PauseLuaScript();

// 恢复Lua脚本
GAMEASSISTANTDLL_API void ResumeLuaScript();

// 停止Lua脚本
GAMEASSISTANTDLL_API void StopLuaScript();

// 获取脚本运行状态
GAMEASSISTANTDLL_API BOOL IsLuaScriptRunning();

// 获取脚本暂停状态
GAMEASSISTANTDLL_API BOOL IsLuaScriptPaused();

// 上报账号信息
GAMEASSISTANTDLL_API void ReportAccountInfo(const AccountInfoPacket* pAccountInfo);

// 上报脚本状态
GAMEASSISTANTDLL_API void ReportScriptStatus(const ScriptStatusPacket* pScriptStatus);

#endif // GAME_ASSISTANT_DLL_H