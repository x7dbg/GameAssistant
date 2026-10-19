#pragma once
#ifndef RECOVERY_MANAGER_H
#define RECOVERY_MANAGER_H

// RecoveryManager.h : 错误恢复与自动恢复机制

#include "stdafx.h"
#include "GameAssistantComm.h"
#include <map>
#include <functional>

// 恢复操作类型
enum RecoveryActionType
{
    RECOVERY_NONE = 0,
    RECOVERY_RETRY_INJECT,      // 重试DLL注入
    RECOVERY_RESTART_PROCESS,   // 重启游戏进程
    RECOVERY_RECONNECT,         // 重新连接服务器
    RECOVERY_NOTIFY_USER        // 仅通知用户
};

// 恢复配置
struct RecoveryConfig
{
    int nMaxRetryCount;         // 最大重试次数
    int nRetryInterval;         // 重试间隔（毫秒）
    BOOL bAutoRestartProcess;   // 自动重启进程
    BOOL bAutoReconnect;        // 自动重连
    int nProcessRestartDelay;   // 进程重启延迟（毫秒）

    RecoveryConfig() 
        : nMaxRetryCount(3)
        , nRetryInterval(5000)
        , bAutoRestartProcess(TRUE)
        , bAutoReconnect(TRUE)
        , nProcessRestartDelay(10000)
    {}
};

// 每个账号的恢复记录
struct RecoveryRecord
{
    DWORD dwAccountIndex;       // 账号索引
    DWORD dwPID;                // 进程ID
    int nRetryCount;            // 当前重试次数
    DWORD dwLastErrorTime;      // 上次错误时间
    RecoveryActionType eLastAction; // 上次恢复操作
    BOOL bInRecovery;           // 当前是否在恢复中

    RecoveryRecord() 
        : dwAccountIndex(0)
        , dwPID(0)
        , nRetryCount(0)
        , dwLastErrorTime(0)
        , eLastAction(RECOVERY_NONE)
        , bInRecovery(FALSE)
    {}
};

// 恢复回调函数类型
typedef std::function<void(DWORD accountIndex, RecoveryActionType action, BOOL success)> RecoveryCallback;

// CRecoveryManager 类
class CRecoveryManager
{
public:
    CRecoveryManager();
    virtual ~CRecoveryManager();

    // 用配置初始化
    void Initialize(const RecoveryConfig& config);

    // 注册账号进行监控
    void RegisterAccount(DWORD dwAccountIndex, DWORD dwPID);

    // 注销账号
    void UnregisterAccount(DWORD dwAccountIndex);

    // 报告错误并触发恢复
    BOOL ReportError(DWORD dwAccountIndex, ErrorCode errorCode, const CString& strContext);

    // 手动重试指定账号
    BOOL ManualRetry(DWORD dwAccountIndex);

    // 设置恢复回调
    void SetRecoveryCallback(RecoveryCallback callback)
    {
        m_recoveryCallback = callback;
    }

    // 获取恢复记录
    BOOL GetRecoveryRecord(DWORD dwAccountIndex, RecoveryRecord& record);

    // 重置重试次数
    void ResetRetryCount(DWORD dwAccountIndex);

    // 检查账号是否在恢复中
    BOOL IsInRecovery(DWORD dwAccountIndex);

    // 更新进程ID（重启后）
    void UpdateProcessID(DWORD dwAccountIndex, DWORD dwNewPID);

    // 启动/停止监控线程
    void StartMonitoring();
    void StopMonitoring();

private:
    RecoveryConfig m_config;
    std::map<DWORD, RecoveryRecord> m_recoveryMap;
    CCriticalSection m_csMap;
    RecoveryCallback m_recoveryCallback;
    CWinThread* m_pMonitorThread;
    BOOL m_bMonitoring;

    // 恢复操作
    BOOL PerformRecovery(DWORD dwAccountIndex, ErrorCode errorCode);
    BOOL RetryInjection(DWORD dwAccountIndex);
    BOOL RestartProcess(DWORD dwAccountIndex);
    BOOL ReconnectServer(DWORD dwAccountIndex);

    // 监控线程
    static UINT MonitorThreadProc(LPVOID pParam);
    void MonitorLoop();

    // 辅助函数
    RecoveryActionType DetermineRecoveryAction(ErrorCode errorCode);
    BOOL ShouldAttemptRecovery(const RecoveryRecord& record);
    void NotifyRecoveryResult(DWORD dwAccountIndex, RecoveryActionType action, BOOL success);
};

#endif // RECOVERY_MANAGER_H