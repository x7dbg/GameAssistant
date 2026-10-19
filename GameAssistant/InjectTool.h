#pragma once
#ifndef INJECT_TOOL_H
#define INJECT_TOOL_H

#include "stdafx.h"

// InjectTool 类 - DLL注入工具
class InjectTool
{
public:
    // 远程线程注入（支持挂起注入+恢复）
    static BOOL RemoteInject(DWORD processID, const CString& dllPath, BOOL bResumeAfterInject = FALSE);
    
    // 挂起状态下的远程注入
    static BOOL RemoteInjectSuspended(HANDLE hProcess, const CString& dllPath);
    
    // 手动映射注入（高级功能）
    static BOOL ManualMapInject(DWORD processID, const CString& dllPath);
};

#endif // INJECT_TOOL_H