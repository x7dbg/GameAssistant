// RecoveryManager.cpp : 错误恢复实现

#include "stdafx.h"
#include "RecoveryManager.h"
#include "InjectTool.h"

CRecoveryManager::CRecoveryManager()
    : m_pMonitorThread(NULL)
    , m_bMonitoring(FALSE)
{
}

CRecoveryManager::~CRecoveryManager()
{
    StopMonitoring();
}

void CRecoveryManager::Initialize(const RecoveryConfig& config)
{
    m_config = config;
}

void CRecoveryManager::RegisterAccount(DWORD dwAccountIndex, DWORD dwPID)
{
    CSingleLock lock(&m_csMap, TRUE);

    RecoveryRecord record;
    record.dwAccountIndex = dwAccountIndex;
    record.dwPID = dwPID;
    record.nRetryCount = 0;
    record.dwLastErrorTime = 0;
    record.eLastAction = RECOVERY_NONE;
    record.bInRecovery = FALSE;

    m_recoveryMap[dwAccountIndex] = record;
}

void CRecoveryManager::UnregisterAccount(DWORD dwAccountIndex)
{
    CSingleLock lock(&m_csMap, TRUE);
    m_recoveryMap.erase(dwAccountIndex);
}

BOOL CRecoveryManager::ReportError(DWORD dwAccountIndex, ErrorCode errorCode, const CString& strContext)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it == m_recoveryMap.end())
    {
        return FALSE;
    }

    RecoveryRecord& record = it->second;
    record.dwLastErrorTime = GetTickCount();

    // 检查是否应尝试恢复
    if (!ShouldAttemptRecovery(record))
    {
        // 已达最大重试次数，通知用户
        NotifyRecoveryResult(dwAccountIndex, RECOVERY_NOTIFY_USER, FALSE);
        return FALSE;
    }

    lock.Unlock();

    // 执行恢复
    return PerformRecovery(dwAccountIndex, errorCode);
}

BOOL CRecoveryManager::ManualRetry(DWORD dwAccountIndex)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it == m_recoveryMap.end())
    {
        return FALSE;
    }

    it->second.nRetryCount = 0;
    it->second.bInRecovery = FALSE;

    lock.Unlock();

    return PerformRecovery(dwAccountIndex, ERROR_PROCESS_OP_FAILED);
}

BOOL CRecoveryManager::GetRecoveryRecord(DWORD dwAccountIndex, RecoveryRecord& record)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it != m_recoveryMap.end())
    {
        record = it->second;
        return TRUE;
    }

    return FALSE;
}

void CRecoveryManager::ResetRetryCount(DWORD dwAccountIndex)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it != m_recoveryMap.end())
    {
        it->second.nRetryCount = 0;
    }
}

BOOL CRecoveryManager::IsInRecovery(DWORD dwAccountIndex)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it != m_recoveryMap.end())
    {
        return it->second.bInRecovery;
    }

    return FALSE;
}

void CRecoveryManager::UpdateProcessID(DWORD dwAccountIndex, DWORD dwNewPID)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it != m_recoveryMap.end())
    {
        it->second.dwPID = dwNewPID;
        it->second.nRetryCount = 0; // 成功重启后重置重试次数
    }
}

void CRecoveryManager::StartMonitoring()
{
    if (m_bMonitoring)
    {
        return;
    }

    m_bMonitoring = TRUE;
    m_pMonitorThread = AfxBeginThread(MonitorThreadProc, this, 
        THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
    
    if (m_pMonitorThread)
    {
        m_pMonitorThread->ResumeThread();
    }
}

void CRecoveryManager::StopMonitoring()
{
    m_bMonitoring = FALSE;

    if (m_pMonitorThread)
    {
        WaitForSingleObject(m_pMonitorThread->m_hThread, 5000);
        m_pMonitorThread = NULL;
    }
}

UINT CRecoveryManager::MonitorThreadProc(LPVOID pParam)
{
    CRecoveryManager* pManager = (CRecoveryManager*)pParam;
    pManager->MonitorLoop();
    return 0;
}

void CRecoveryManager::MonitorLoop()
{
    while (m_bMonitoring)
    {
        Sleep(1000); // 每秒检查一次

        CSingleLock lock(&m_csMap, TRUE);
        DWORD dwCurrentTime = GetTickCount();

        for (auto& pair : m_recoveryMap)
        {
            RecoveryRecord& record = pair.second;

            // 检查进程是否仍在运行
            if (record.dwPID > 0 && record.bInRecovery == FALSE)
            {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, record.dwPID);
                if (hProcess == NULL)
                {
                    // 进程崩溃或终止
                    lock.Unlock();
                    ReportError(pair.first, ERROR_PROCESS_OP_FAILED, _T("Process crashed"));
                    lock.Lock();
                } else
                {
                    CloseHandle(hProcess);
                }
            }

            // 检查待处理的重连
            if (record.eLastAction == RECOVERY_RECONNECT && 
                dwCurrentTime - record.dwLastErrorTime > (DWORD)m_config.nRetryInterval)
                {
                // 尝试重连
                lock.Unlock();
                ReconnectServer(pair.first);
                lock.Lock();
            }
        }
    }
}

BOOL CRecoveryManager::PerformRecovery(DWORD dwAccountIndex, ErrorCode errorCode)
{
    CSingleLock lock(&m_csMap, TRUE);

    auto it = m_recoveryMap.find(dwAccountIndex);
    if (it == m_recoveryMap.end())
    {
        return FALSE;
    }

    RecoveryRecord& record = it->second;
    record.bInRecovery = TRUE;
    record.nRetryCount++;

    RecoveryActionType action = DetermineRecoveryAction(errorCode);
    record.eLastAction = action;

    lock.Unlock();

    BOOL bSuccess = FALSE;

    switch (action)
    {
    case RECOVERY_RETRY_INJECT:
        bSuccess = RetryInjection(dwAccountIndex);
        break;

    case RECOVERY_RESTART_PROCESS:
        bSuccess = RestartProcess(dwAccountIndex);
        break;

    case RECOVERY_RECONNECT:
        bSuccess = ReconnectServer(dwAccountIndex);
        break;

    case RECOVERY_NOTIFY_USER:
    default:
        // 仅通知用户
        bSuccess = FALSE;
        break;
    }

    // 更新恢复状态
    lock.Lock();
    it = m_recoveryMap.find(dwAccountIndex);
    if (it != m_recoveryMap.end())
    {
        it->second.bInRecovery = FALSE;
        if (bSuccess)
        {
            it->second.nRetryCount = 0; // 成功时重置
        }
    }
    lock.Unlock();

    NotifyRecoveryResult(dwAccountIndex, action, bSuccess);

    return bSuccess;
}

BOOL CRecoveryManager::RetryInjection(DWORD dwAccountIndex)
{
    // 这将从主对话框调用
    // 目前仅触发回调
    NotifyRecoveryResult(dwAccountIndex, RECOVERY_RETRY_INJECT, TRUE);
    return TRUE;
}

BOOL CRecoveryManager::RestartProcess(DWORD dwAccountIndex)
{
    // 重启前延迟
    Sleep(m_config.nProcessRestartDelay);

    // 通知主对话框重启进程
    NotifyRecoveryResult(dwAccountIndex, RECOVERY_RESTART_PROCESS, TRUE);
    return TRUE;
}

BOOL CRecoveryManager::ReconnectServer(DWORD dwAccountIndex)
{
    // 这将重新连接游戏客户端到服务器
    NotifyRecoveryResult(dwAccountIndex, RECOVERY_RECONNECT, TRUE);
    return TRUE;
}

RecoveryActionType CRecoveryManager::DetermineRecoveryAction(ErrorCode errorCode)
{
    switch (errorCode)
    {
    case ERROR_PROCESS_OP_FAILED:
        return RECOVERY_RESTART_PROCESS;

    case ERROR_MEMORY_ALLOC_FAILED:
    case ERROR_DATA_SEND_FAILED:
        return RECOVERY_RETRY_INJECT;

    case ERROR_THREAD_CREATE_FAILED:
        return RECOVERY_RECONNECT;

    default:
        return RECOVERY_NOTIFY_USER;
    }
}

BOOL CRecoveryManager::ShouldAttemptRecovery(const RecoveryRecord& record)
{
    if (record.nRetryCount >= m_config.nMaxRetryCount)
    {
        return FALSE;
    }

    // 检查距上次尝试是否已过足够时间
    DWORD dwCurrentTime = GetTickCount();
    if (dwCurrentTime - record.dwLastErrorTime < (DWORD)m_config.nRetryInterval)
    {
        return FALSE;
    }

    return TRUE;
}

void CRecoveryManager::NotifyRecoveryResult(DWORD dwAccountIndex, RecoveryActionType action, BOOL success)
{
    if (m_recoveryCallback)
    {
        m_recoveryCallback(dwAccountIndex, action, success);
    }
}
